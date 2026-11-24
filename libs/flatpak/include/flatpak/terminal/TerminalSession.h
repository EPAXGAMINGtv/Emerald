#pragma once

#include <functional>
#include <string>
#include <vector>

#include "flatpak/FlatpakTypes.h"

namespace fpk::terminal {

using InteractiveExecutor =
    std::function<Result(const std::vector<std::string>&)>;

struct TerminalOptions {
    std::string Shell;
    std::string WorkingDirectory;
};

// Runs commands with inherited stdin/stdout/stderr. This is the bridge for a
// GUI app store that wants a real Flatpak console inside its window.
class TerminalSession {
public:
    explicit TerminalSession(
        TerminalOptions options = {}, InteractiveExecutor executor = {}
    );

    void SetOptions(TerminalOptions options);
    const TerminalOptions& Options() const noexcept;

    void SetExecutor(InteractiveExecutor executor);

    std::string ResolveShell() const;
    std::vector<std::string> ShellArguments() const;

    Result RunInteractive(std::vector<std::string> arguments) const;
    Result OpenShell() const;

private:
    TerminalOptions options_;
    InteractiveExecutor executor_;
};

} // namespace fpk::terminal
