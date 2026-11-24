#include "FlatpakParse.h"

namespace fpk::detail {

namespace {

std::vector<std::string> SplitBy(
    const std::string& input, char delimiter, bool keepTrailingEmpty
) {
    std::vector<std::string> parts;
    std::size_t start = 0U;

    for (;;) {
        const std::size_t found = input.find(delimiter, start);
        if (found == std::string::npos) {
            const std::string tail = input.substr(start);
            if (!tail.empty() || keepTrailingEmpty) {
                parts.push_back(tail);
            }
            break;
        }

        parts.push_back(input.substr(start, found - start));
        start = found + 1U;
    }

    return parts;
}

} // namespace

std::vector<std::string> SplitLines(const std::string& output) {
    std::vector<std::string> lines = SplitBy(output, '\n', false);
    for (std::string& line : lines) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
    }
    return lines;
}

std::vector<std::string> SplitTabs(const std::string& line) {
    return SplitBy(line, '\t', true);
}

std::string Trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1U);
}

} // namespace fpk::detail
