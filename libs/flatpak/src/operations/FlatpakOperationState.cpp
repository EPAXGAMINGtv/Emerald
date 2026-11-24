#include "operations/FlatpakOperationState.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fpk::ops {

namespace {

using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;

bool IsDigit(char value) {
    return std::isdigit(static_cast<unsigned char>(value)) != 0;
}

bool IsSpace(char value) {
    return std::isspace(static_cast<unsigned char>(value)) != 0;
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

} // namespace

Result Failure(std::string message) {
    Result result;
    result.Error = std::move(message);
    return result;
}

Progress EmptyProgress(const std::string& id) {
    Progress progress;
    progress.PackageId = id;
    progress.Phase = "none";
    return progress;
}

void Start(
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

void Finish(
    OperationMap& operations,
    const std::string& id,
    const std::string& phase,
    const Result& result
) {
    auto found = operations.find(id);
    if (found == operations.end()) {
        Start(operations, id, phase);
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

} // namespace fpk::ops
