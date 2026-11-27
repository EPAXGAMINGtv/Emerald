#include "flatpak/terminal/TerminalSession.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <sys/wait.h>
#include <unistd.h>

namespace fpk::terminal {

TerminalSession::TerminalSession(
    TerminalOptions options, InteractiveExecutor executor
)
    : options_(std::move(options)), executor_(std::move(executor)) {}

void TerminalSession::SetOptions(TerminalOptions options) {
    options_ = std::move(options);
}

const TerminalOptions& TerminalSession::Options() const noexcept {
    return options_;
}

void TerminalSession::SetExecutor(InteractiveExecutor executor) {
    executor_ = std::move(executor);
}

std::string TerminalSession::ResolveShell() const {
    if (!options_.Shell.empty()) {
        return options_.Shell;
    }

    const char* environmentShell = std::getenv("SHELL");
    if (environmentShell != nullptr && environmentShell[0] != '\0') {
        return environmentShell;
    }
    return "/bin/sh";
}

std::vector<std::string> TerminalSession::ShellArguments() const {
    return {ResolveShell(), "-i"};
}

Result TerminalSession::RunInteractive(
    std::vector<std::string> arguments
) const {
    Result result;
    result.Command = arguments;

    const auto started = std::chrono::steady_clock::now();
    if (arguments.empty()) {
        result.Error = "empty terminal command";
        return result;
    }

    if (executor_) {
        result = executor_(arguments);
        if (result.Command.empty()) {
            result.Command = arguments;
        }
        return result;
    }

    const pid_t child = ::fork();
    if (child < 0) {
        result.Error = std::strerror(errno);
        return result;
    }

    if (child == 0) {
        if (!options_.WorkingDirectory.empty() &&
            ::chdir(options_.WorkingDirectory.c_str()) != 0) {
            const std::string failure =
                std::string("chdir failed: ") + std::strerror(errno);
            const ssize_t written =
                ::write(STDOUT_FILENO, failure.data(), failure.size());
            (void)written;
            ::_exit(127);
        }

        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1U);
        for (std::string& argument : arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);

        ::execvp(arguments[0].c_str(), argv.data());
        const std::string failure =
            std::string("execvp failed: ") + std::strerror(errno);
        const ssize_t written =
            ::write(STDOUT_FILENO, failure.data(), failure.size());
        (void)written;
        ::_exit(127);
    }

    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }

    result.Duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    );
    if (WIFEXITED(status)) {
        result.ExitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.ExitCode = 128 + WTERMSIG(status);
    } else {
        result.ExitCode = -1;
    }

    result.Ok = result.ExitCode == 0;
    if (!result.Ok) {
        result.Error = "terminal command exited with code " +
            std::to_string(result.ExitCode);
    }
    return result;
}

Result TerminalSession::OpenShell() const {
    return RunInteractive(ShellArguments());
}

} // namespace fpk::terminal
