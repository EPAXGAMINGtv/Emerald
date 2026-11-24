#pragma once

#include <string>
#include <vector>

#include "flatpak/FlatpakCommand.h"
#include "flatpak/FlatpakTypes.h"

namespace fpk {

// Central facade for the Flatpak app-store library. More grouped APIs are
// added on this class, so callers only need one object.
class Flatpak {
public:
    explicit Flatpak(
        std::string binaryPath = "flatpak",
        CommandExecutor executor = {}
    );

    void SetBinaryPath(std::string binaryPath);
    const std::string& BinaryPath() const noexcept;
    void SetExecutor(CommandExecutor executor);

    bool IsAvailable() const;
    std::string Version() const;
    Result EnsureAvailable() const;

    std::vector<std::string> DefaultRepositories() const;

private:
    FlatpakCommand command_;
};

} // namespace fpk
