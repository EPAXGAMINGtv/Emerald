#include "flatpak/FlatpakTypes.h"

namespace fpk {

const char* ToString(OperationStatus status) noexcept {
    switch (status) {
        case OperationStatus::None:
            return "none";
        case OperationStatus::Queued:
            return "queued";
        case OperationStatus::Running:
            return "running";
        case OperationStatus::Succeeded:
            return "succeeded";
        case OperationStatus::Failed:
            return "failed";
        case OperationStatus::Cancelled:
            return "cancelled";
        default:
            break;
    }
    return "unknown";
}

} // namespace fpk
