#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fpk {

struct Result {
    bool Ok = false;
    int ExitCode = -1;
    std::string Output;
    std::string Error;
    std::vector<std::string> Command;
    std::chrono::milliseconds Duration{0};
};

struct Application {
    std::string Id;
    std::string Name;
    std::string Summary;
    std::string Description;
    std::string Version;
    std::string Branch;
    std::string Remote;
    std::string License;
    std::vector<std::string> Categories;
    std::vector<std::string> Groups;
    std::filesystem::path IconPath;
};

struct Repository {
    std::string Name;
    std::string Url;
    bool Enabled = true;
    int Priority = 0;
};

struct Group {
    std::string Id;
    std::string Name;
    std::vector<std::string> ApplicationIds;
};

struct Progress {
    std::string PackageId;
    std::string Phase;
    double Fraction = 0.0;
    std::chrono::milliseconds Elapsed{0};
    std::optional<std::chrono::milliseconds> Remaining;
};

enum class OperationStatus {
    None,
    Queued,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

struct Operation {
    std::string PackageId;
    OperationStatus Status = OperationStatus::None;
    Progress Current;
    std::chrono::steady_clock::time_point Started{};
};

const char* ToString(OperationStatus status) noexcept;

} // namespace fpk
