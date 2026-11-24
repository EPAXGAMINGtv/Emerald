#pragma once

#include <string>
#include <vector>

namespace fpk::detail {

std::vector<std::string> SplitLines(const std::string& output);
std::vector<std::string> SplitTabs(const std::string& line);

} // namespace fpk::detail
