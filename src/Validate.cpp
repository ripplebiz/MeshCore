#include "Validate.h"

namespace Validate {
    bool validEpochTime(uint32_t timestamp) {
        // Basic validation: must be positive and not max uint32 value
        return (timestamp > 0 && timestamp < UINT32_MAX);
    }
}