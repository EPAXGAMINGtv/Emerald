#pragma once

#include <functional>
#include <string>
#include <vector>

#include "flatpak/FlatpakTypes.h"
#include "flatpak/streaming/OutputCallback.h"

namespace fpk {

using CommandExecutor = std::function<Result(const std::vector<std::string>&)>;
using StreamingExecutor = std::function<Result(
    const std::vector<std::string>&, const stream::OutputCallback&
)>;

// Executes flatpak commands as argv vectors. No shell string is ever built, so
// package ids and repository urls cannot be interpreted as shell syntax.
class FlatpakCommand {
public:
    explicit FlatpakCommand(
        std::string binary = "flatpak",
        CommandExecutor executor = {}
    );

    void SetBinaryPath(std::string binary);
    const std::string& BinaryPath() const noexcept;

    void SetExecutor(CommandExecutor executor);
    void SetStreamingExecutor(StreamingExecutor executor);

    Result Run(
        std::vector<std::string> arguments,
        stream::OutputCallback onOutput = {}
    ) const;

private:
    std::string binary_;
    CommandExecutor executor_;
    StreamingExecutor streamingExecutor_;
};

} // namespace fpk
