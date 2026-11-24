#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace fpk::stream {

// Receives terminal chunks as soon as they are produced by a child process.
using OutputCallback = std::function<void(const std::string&)>;

// Turns raw process chunks into line/carriage-return events. Flatpak prints
// progress with '\r', so both '\r' and '\n' are treated as delimiters.
class OutputLineBuffer {
public:
    explicit OutputLineBuffer(OutputCallback callback = {});

    void Push(std::string_view chunk);
    void Flush();

private:
    OutputCallback callback_;
    std::string pending_;
    char lastDelimiter_ = '\0';
};

} // namespace fpk::stream
