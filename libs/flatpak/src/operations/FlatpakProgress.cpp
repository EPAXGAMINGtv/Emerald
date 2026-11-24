#include "flatpak/Flatpak.h"

#include "operations/FlatpakOperationState.h"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace fpk {

using Milliseconds = std::chrono::milliseconds;

Progress Flatpak::GetPackageInstallProgress(
    const std::string& packageId
) const {
    const auto found = operations_.find(packageId);
    if (found == operations_.end()) {
        return ops::EmptyProgress(packageId);
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

    ops::Start(operations_, packageId, "queued");
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
