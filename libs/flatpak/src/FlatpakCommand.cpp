#include "flatpak/FlatpakCommand.h"

#include <cerrno>
#include <cstring>
#include <utility>

#include <sys/wait.h>
#include <unistd.h>

namespace fpk {

namespace {

Result RunProcess(const std::vector<std::string>& command) {
    Result result;
    result.Command = command;

    const auto started = std::chrono::steady_clock::now();
    if (command.empty()) {
        result.Error = "empty flatpak command";
        return result;
    }

    int pipe_fd[2] = {-1, -1};
    if (::pipe(pipe_fd) != 0) {
        result.Error = std::strerror(errno);
        return result;
    }

    const pid_t child = ::fork();
    if (child < 0) {
        result.Error = std::strerror(errno);
        (void)::close(pipe_fd[0]);
        (void)::close(pipe_fd[1]);
        return result;
    }

    if (child == 0) {
        (void)::close(pipe_fd[0]);
        (void)::dup2(pipe_fd[1], STDOUT_FILENO);
        (void)::dup2(pipe_fd[1], STDERR_FILENO);
        (void)::close(pipe_fd[1]);

        std::vector<std::string> arguments = command;
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1U);
        for (std::string& argument : arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);

        ::execvp(arguments[0].c_str(), argv.data());
        const std::string failure =
            std::string("execvp failed: ") + std::strerror(errno);
        (void)::write(STDOUT_FILENO, failure.data(), failure.size());
        ::_exit(127);
    }

    (void)::close(pipe_fd[1]);

    std::string output;
    char buffer[4096] = {};
    for (;;) {
        const ssize_t count = ::read(pipe_fd[0], buffer, sizeof(buffer));
        if (count > 0) {
            output.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
    (void)::close(pipe_fd[0]);

    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }

    result.Duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    );
    result.Output = output;

    if (WIFEXITED(status)) {
        result.ExitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.ExitCode = 128 + WTERMSIG(status);
    } else {
        result.ExitCode = -1;
    }

    result.Ok = result.ExitCode == 0;
    if (!result.Ok) {
        result.Error = "flatpak exited with code " +
            std::to_string(result.ExitCode);
    }
    return result;
}

} // namespace

FlatpakCommand::FlatpakCommand(
    std::string binary, CommandExecutor executor
)
    : binary_(std::move(binary)), executor_(std::move(executor)) {}

void FlatpakCommand::SetBinaryPath(std::string binary) {
    binary_ = std::move(binary);
}

const std::string& FlatpakCommand::BinaryPath() const noexcept {
    return binary_;
}

void FlatpakCommand::SetExecutor(CommandExecutor executor) {
    executor_ = std::move(executor);
}

Result FlatpakCommand::Run(std::vector<std::string> arguments) const {
    std::vector<std::string> command;
    command.reserve(arguments.size() + 1U);
    command.push_back(binary_);
    command.insert(command.end(), arguments.begin(), arguments.end());

    Result result;
    result.Command = command;

    if (binary_.empty()) {
        result.Error = "empty flatpak binary path";
        return result;
    }

    if (executor_) {
        result = executor_(command);
        if (result.Command.empty()) {
            result.Command = command;
        }
        return result;
    }

    return RunProcess(command);
}

} // namespace fpk
