#include "flatpak/Flatpak.h"

#include "FlatpakParse.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fpk {

namespace {

using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;
using OperationMap = std::unordered_map<std::string, Operation>;

bool IsDigit(char value) {
    return std::isdigit(static_cast<unsigned char>(value)) != 0;
}

bool IsSpace(char value) {
    return std::isspace(static_cast<unsigned char>(value)) != 0;
}

Result Failure(std::string message) {
    Result result;
    result.Error = std::move(message);
    return result;
}

double ParseFraction(const std::string& output) {
    double fraction = 0.0;

    for (std::size_t index = 0U; index < output.size(); ++index) {
        if (output[index] != '%' || index == 0U) {
            continue;
        }

        std::size_t start = index;
        while (start > 0U && IsDigit(output[start - 1U])) {
            --start;
        }
        if (start == index) {
            continue;
        }

        try {
            fraction = std::stod(output.substr(start, index - start)) / 100.0;
        } catch (const std::exception&) {
            continue;
        }
    }

    return std::clamp(fraction, 0.0, 1.0);
}

std::optional<Milliseconds> ParseDurationToken(const std::string& token) {
    const std::size_t first = token.find_first_of("0123456789");
    if (first == std::string::npos) {
        return std::nullopt;
    }

    std::vector<long long> numbers;
    std::size_t position = first;
    while (position < token.size()) {
        long long value = 0;
        while (position < token.size() && IsDigit(token[position])) {
            value = value * 10 + (token[position] - '0');
            ++position;
        }
        numbers.push_back(value);

        if (position < token.size() && token[position] == ':') {
            ++position;
            continue;
        }
        break;
    }

    long long seconds = 0;
    if (numbers.size() == 1U) {
        const std::string suffix = token.substr(position);
        if (suffix.find("ms") != std::string::npos) {
            return Milliseconds(numbers[0]);
        }
        if (suffix.find("min") != std::string::npos ||
            suffix.find('m') != std::string::npos) {
            seconds = numbers[0] * 60;
        } else if (suffix.find('h') != std::string::npos) {
            seconds = numbers[0] * 3600;
        } else {
            seconds = numbers[0];
        }
    } else if (numbers.size() == 2U) {
        seconds = numbers[0] * 60 + numbers[1];
    } else if (numbers.size() == 3U) {
        seconds = numbers[0] * 3600 + numbers[1] * 60 + numbers[2];
    } else {
        return std::nullopt;
    }

    return Milliseconds(seconds * 1000);
}

std::optional<Milliseconds> ParseRemaining(const std::string& output) {
    const std::size_t marker = output.find("remaining");
    if (marker == std::string::npos) {
        return std::nullopt;
    }

    const std::size_t after = output.find_first_of("0123456789", marker);
    if (after != std::string::npos) {
        std::size_t end = after;
        while (end < output.size() && !IsSpace(output[end])) {
            ++end;
        }
        return ParseDurationToken(output.substr(after, end - after));
    }

    std::size_t end = marker;
    while (end > 0U && IsSpace(output[end - 1U])) {
        --end;
    }
    std::size_t start = end;
    while (start > 0U && !IsSpace(output[start - 1U])) {
        --start;
    }
    if (start == end) {
        return std::nullopt;
    }
    return ParseDurationToken(output.substr(start, end - start));
}

Progress EmptyProgress(const std::string& id) {
    Progress progress;
    progress.PackageId = id;
    progress.Phase = "none";
    return progress;
}

void StartOperation(
    OperationMap& operations, const std::string& id, const std::string& phase
) {
    Operation operation;
    operation.PackageId = id;
    operation.Status = OperationStatus::Running;
    operation.Started = Clock::now();
    operation.Current = EmptyProgress(id);
    operation.Current.Phase = phase;
    operations[id] = std::move(operation);
}

void FinishOperation(
    OperationMap& operations,
    const std::string& id,
    const std::string& phase,
    const Result& result
) {
    auto found = operations.find(id);
    if (found == operations.end()) {
        StartOperation(operations, id, phase);
        found = operations.find(id);
    }

    Operation& operation = found->second;
    operation.Status = result.Ok ? OperationStatus::Succeeded
                                 : OperationStatus::Failed;
    operation.Current = EmptyProgress(id);
    operation.Current.Phase = phase;
    operation.Current.Fraction = ParseFraction(result.Output);
    operation.Current.Remaining = ParseRemaining(result.Output);

    const Milliseconds measured = std::chrono::duration_cast<Milliseconds>(
        Clock::now() - operation.Started
    );
    operation.Current.Elapsed =
        result.Duration.count() > 0 ? result.Duration : measured;
}

std::vector<std::string> SplitRef(const std::string& ref) {
    std::vector<std::string> parts;
    std::size_t start = 0U;

    for (;;) {
        const std::size_t found = ref.find('/', start);
        if (found == std::string::npos) {
            parts.push_back(ref.substr(start));
            break;
        }
        parts.push_back(ref.substr(start, found - start));
        start = found + 1U;
    }

    return parts;
}

} // namespace

Result Flatpak::InstallFlatpakPackage(
    const std::string& packageId,
    const std::string& remote,
    const std::string& branch
) {
    if (packageId.empty() || remote.empty()) {
        return Failure("package id and remote are required");
    }

    std::vector<std::string> arguments = {
        "install",
        "-y",
        "--noninteractive",
        remote,
        packageId
    };
    if (!branch.empty()) {
        arguments.push_back(branch);
    }

    StartOperation(operations_, packageId, "install");
    Result result = command_.Run(std::move(arguments));
    FinishOperation(operations_, packageId, "install", result);
    return result;
}

Result Flatpak::InstallFlatpakPackageFromRef(const std::string& ref) {
    const std::vector<std::string> parts = SplitRef(ref);
    if (parts.size() == 1U) {
        return InstallFlatpakPackage(parts[0]);
    }
    if (parts.size() == 2U) {
        return InstallFlatpakPackage(parts[1], parts[0]);
    }
    if (parts.size() == 3U) {
        return InstallFlatpakPackage(parts[1], parts[0], parts[2]);
    }
    return Failure("invalid flatpak ref: " + ref);
}

Result Flatpak::UninstallFlatpakPackage(const std::string& packageId) {
    if (packageId.empty()) {
        return Failure("package id is required");
    }

    StartOperation(operations_, packageId, "uninstall");
    Result result = command_.Run({
        "uninstall", "-y", "--noninteractive", packageId
    });
    FinishOperation(operations_, packageId, "uninstall", result);
    return result;
}

Result Flatpak::UpdateFlatpakPackage(const std::string& packageId) {
    if (packageId.empty()) {
        return Failure("package id is required");
    }

    StartOperation(operations_, packageId, "update");
    Result result = command_.Run({
        "update", "-y", "--noninteractive", packageId
    });
    FinishOperation(operations_, packageId, "update", result);
    return result;
}

Result Flatpak::UpdateAllFlatpakPackages() {
    StartOperation(operations_, "all", "update");
    Result result = command_.Run({"update", "-y", "--noninteractive"});
    FinishOperation(operations_, "all", "update", result);
    return result;
}

std::vector<std::string> Flatpak::ListFlatpakUpdates() const {
    const Result result = command_.Run({
        "remote-ls",
        "--updates",
        "--app",
        "--columns=application"
    });
    if (!result.Ok) {
        return {};
    }

    std::vector<std::string> updates;
    for (const std::string& line : detail::SplitLines(result.Output)) {
        if (!line.empty()) {
            updates.push_back(line);
        }
    }
    return updates;
}

Result Flatpak::RunFlatpakPackage(
    const std::string& packageId, const std::vector<std::string>& arguments
) {
    if (packageId.empty()) {
        return Failure("package id is required");
    }

    std::vector<std::string> argv = {"run", packageId};
    argv.insert(argv.end(), arguments.begin(), arguments.end());
    return command_.Run(std::move(argv));
}

Progress Flatpak::GetPackageInstallProgress(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end()) {
        return EmptyProgress(packageId);
    }
    return found->second.Current;
}

std::optional<Milliseconds>
Flatpak::GetRemainingPackageInstallTimeForPackage(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end() ||
        !found->second.Current.Remaining.has_value()) {
        return std::nullopt;
    }
    return found->second.Current.Remaining;
}

std::optional<Milliseconds>
Flatpak::GetElapsedPackageInstallTimeForPackage(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end()) {
        return std::nullopt;
    }
    return found->second.Current.Elapsed;
}

double Flatpak::GetPackageInstallFraction(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end()) {
        return 0.0;
    }
    return found->second.Current.Fraction;
}

OperationStatus Flatpak::GetPackageOperationStatus(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end()) {
        return OperationStatus::None;
    }
    return found->second.Status;
}

std::vector<Operation> Flatpak::ListPackageOperations() const {
    std::vector<Operation> operations;
    operations.reserve(operations_.size());
    for (const auto& entry : operations_) {
        operations.push_back(entry.second);
    }
    return operations;
}

bool Flatpak::IsPackageOperationRunning(const std::string& packageId) const {
    const OperationStatus status = GetPackageOperationStatus(packageId);
    return status == OperationStatus::Running ||
           status == OperationStatus::Queued;
}

bool Flatpak::QueueFlatpakPackageOperation(const std::string& packageId) {
    if (packageId.empty() || IsPackageOperationRunning(packageId)) {
        return false;
    }

    StartOperation(operations_, packageId, "queued");
    operations_[packageId].Status = OperationStatus::Queued;
    return true;
}

bool Flatpak::CancelPackageOperation(const std::string& packageId) {
    const auto found = operations_.find(packageId);
    if (found == operations_.end() ||
        !IsPackageOperationRunning(packageId)) {
        return false;
    }
    found->second.Status = OperationStatus::Cancelled;
    return true;
}

void Flatpak::ClearPackageOperations() {
    operations_.clear();
}

} // namespace fpk
