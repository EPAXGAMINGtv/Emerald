#include "flatpak/Flatpak.h"

#include "operations/FlatpakOperationState.h"
#include "parsing/FlatpakParse.h"

#include <string>
#include <vector>

namespace fpk {

Result Flatpak::UpdateFlatpakPackage(const std::string& packageId) {
    if (packageId.empty()) {
        return ops::Failure("package id is required");
    }

    ops::Start(operations_, packageId, "update");
    Result result = command_.Run({
        "update", "-y", "--noninteractive", packageId
    });
    ops::Finish(operations_, packageId, "update", result);
    return result;
}

Result Flatpak::UpdateAllFlatpakPackages() {
    ops::Start(operations_, "all", "update");
    Result result = command_.Run({"update", "-y", "--noninteractive"});
    ops::Finish(operations_, "all", "update", result);
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

} // namespace fpk
