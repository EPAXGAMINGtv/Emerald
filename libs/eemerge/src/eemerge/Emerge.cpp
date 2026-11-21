#include "eemerge/Emerge.h"

#include <spdlog/spdlog.h>
#include <utility>

namespace emgr {

Emerge::Emerge(std::string pw) : pw_(std::move(pw)) {}

bool Emerge::EmergeInstalled() {
    spdlog::info("eemerge: EmergeInstalled stub");
    return true;
}

bool Emerge::EmergeSync() {
    spdlog::info("eemerge: EmergeSync stub");
    return true;
}

bool Emerge::EmergeUpdateSystem() {
    spdlog::info("eemerge: EmergeUpdateSystem stub");
    return true;
}

bool Emerge::EmergeUpdatePackage(std::string const& package) {
    spdlog::info("eemerge: EmergeUpdatePackage stub for {}", package);
    return true;
}

bool Emerge::EmergeInstallPackage(std::string const& package) {
    spdlog::info("eemerge: EmergeInstallPackage stub for {}", package);
    packagesInstalling_.push_back(package);
    return true;
}

std::list<std::string> Emerge::EmergeGetPackages() {
    return packagesInstalling_;
}

} // namespace emgr
