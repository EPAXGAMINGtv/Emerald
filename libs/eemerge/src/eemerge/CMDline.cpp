#include "eemerge/CMDline.h"

#include <utility>

namespace emrg {

CMDline::CMDline(std::string workingDirectory) : workingDir_(std::move(workingDirectory)) {}

} // namespace emrg
