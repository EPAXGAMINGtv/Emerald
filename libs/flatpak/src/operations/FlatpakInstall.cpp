#include "flatpak/Flatpak.h"

#include "operations/FlatpakOperationState.h"

#include <string>
#include <utility>
#include <vector>

namespace fpk {

Result Flatpak::InstallFlatpakPackage(
    const std::string& packageId,
    const std::string& remote,
    const std::string& branch,
    stream::OutputCallback onOutput
) {
    if (packageId.empty() || remote.empty()) {
        return ops::Failure("package id and remote are required");
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

    ops::Start(operations_, packageId, "install");
    Result result = command_.Run(
        std::move(arguments),
        ResolveOutputCallback(std::move(onOutput))
    );
    ops::Finish(operations_, packageId, "install", result);
    return result;
}

Result Flatpak::InstallFlatpakPackageFromRef(
    const std::string& ref, stream::OutputCallback onOutput
) {
    const std::vector<std::string> parts = ops::SplitRef(ref);
    if (parts.size() == 1U) {
        return InstallFlatpakPackage(parts[0], "flathub", "stable",
                                     std::move(onOutput));
    }
    if (parts.size() == 2U) {
        return InstallFlatpakPackage(parts[1], parts[0], "stable",
                                     std::move(onOutput));
    }
    if (parts.size() == 3U) {
        return InstallFlatpakPackage(parts[1], parts[0], parts[2],
                                     std::move(onOutput));
    }
    return ops::Failure("invalid flatpak ref: " + ref);
}

Result Flatpak::UninstallFlatpakPackage(const std::string& packageId) {
    if (packageId.empty()) {
        return ops::Failure("package id is required");
    }

    ops::Start(operations_, packageId, "uninstall");
    Result result = command_.Run({
        "uninstall", "-y", "--noninteractive", packageId
    });
    ops::Finish(operations_, packageId, "uninstall", result);
    return result;
}

Result Flatpak::RunFlatpakPackage(
    const std::string& packageId, const std::vector<std::string>& arguments
) {
    if (packageId.empty()) {
        return ops::Failure("package id is required");
    }

    std::vector<std::string> argv = {"run", packageId};
    argv.insert(argv.end(), arguments.begin(), arguments.end());
    return command_.Run(std::move(argv));
}

} // namespace fpk
