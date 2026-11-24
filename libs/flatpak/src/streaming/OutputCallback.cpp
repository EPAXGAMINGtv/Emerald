#include "flatpak/streaming/OutputCallback.h"

#include <utility>

namespace fpk::stream {

OutputLineBuffer::OutputLineBuffer(OutputCallback callback)
    : callback_(std::move(callback)) {}

void OutputLineBuffer::Push(std::string_view chunk) {
    pending_.append(chunk.data(), chunk.size());

    for (;;) {
        const std::size_t found = pending_.find_first_of("\r\n");
        if (found == std::string::npos) {
            break;
        }

        const char delimiter = pending_[found];
        std::string segment = pending_.substr(0U, found);
        pending_.erase(0U, found + 1U);

        const bool crlfTail =
            delimiter == '\n' && lastDelimiter_ == '\r' && segment.empty();
        lastDelimiter_ = delimiter;
        if (crlfTail || !callback_) {
            continue;
        }

        segment.push_back(delimiter);
        callback_(segment);
    }
}

void OutputLineBuffer::Flush() {
    if (pending_.empty() || !callback_) {
        pending_.clear();
        return;
    }

    std::string tail = std::move(pending_);
    pending_.clear();
    lastDelimiter_ = '\0';
    callback_(tail);
}

} // namespace fpk::stream
