#pragma once

#include <string>
#include <vector>

namespace fpk::detail {

std::vector<std::string> SplitLines(const std::string& output);
std::vector<std::string> SplitTabs(const std::string& line);
std::string Trim(const std::string& text);

} // namespace fpk::detail
