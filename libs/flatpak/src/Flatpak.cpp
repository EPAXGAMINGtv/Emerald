#include "flatpak/Flatpak.h"

#include <utility>

namespace fpk {

namespace {

Result MakeResult(bool ok, std::string message) {
    Result result;
    result.Ok = ok;
    if (ok) {
        result.Output = std::move(message);
    } else {
        result.Error = std::move(message);
    }
    return result;
}

} // namespace

Flatpak::Flatpak(std::string binaryPath, CommandExecutor executor)
    : command_(std::move(binaryPath), std::move(executor)) {}

void Flatpak::SetBinaryPath(std::string binaryPath) {
    command_.SetBinaryPath(std::move(binaryPath));
}

const std::string& Flatpak::BinaryPath() const noexcept {
    return command_.BinaryPath();
}

void Flatpak::SetExecutor(CommandExecutor executor) {
    command_.SetExecutor(std::move(executor));
}

void Flatpak::SetStreamingExecutor(StreamingExecutor executor) {
    command_.SetStreamingExecutor(std::move(executor));
}

void Flatpak::SetOutputCallback(stream::OutputCallback callback) {
    outputCallback_ = std::move(callback);
}

const stream::OutputCallback& Flatpak::OutputCallbackHandler() const noexcept {
    return outputCallback_;
}

stream::OutputCallback Flatpak::ResolveOutputCallback(
    stream::OutputCallback explicitCallback
) const {
    return explicitCallback ? explicitCallback : outputCallback_;
}

Result Flatpak::RunFlatpakCommandWithOutput(
    std::vector<std::string> arguments, stream::OutputCallback onOutput
) const {
    return command_.Run(
        std::move(arguments), ResolveOutputCallback(std::move(onOutput))
    );
}

bool Flatpak::IsAvailable() const {
    return !Version().empty();
}

std::string Flatpak::Version() const {
    const Result result = command_.Run({"--version"});
    if (!result.Ok) {
        return {};
    }

    std::string line = result.Output;
    const std::size_t newline = line.find('\n');
    if (newline != std::string::npos) {
        line.erase(newline);
    }

    const std::string prefix = "Flatpak ";
    if (line.starts_with(prefix)) {
        return line.substr(prefix.size());
    }
    return line;
}

Result Flatpak::EnsureAvailable() const {
    const std::string version = Version();
    if (version.empty()) {
        return MakeResult(false, "flatpak is not available");
    }
    return MakeResult(true, version);
}

std::vector<std::string> Flatpak::DefaultRepositories() const {
    return {"flathub"};
}

} // namespace fpk
