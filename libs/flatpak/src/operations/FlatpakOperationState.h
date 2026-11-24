#pragma once

#include "flatpak/FlatpakTypes.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace fpk::ops {

using OperationMap = std::unordered_map<std::string, Operation>;

Result Failure(std::string message);
Progress EmptyProgress(const std::string& id);
void Start(
    OperationMap& operations,
    const std::string& id,
    const std::string& phase
);
void Finish(
    OperationMap& operations,
    const std::string& id,
    const std::string& phase,
    const Result& result
);
std::vector<std::string> SplitRef(const std::string& ref);

} // namespace fpk::ops
