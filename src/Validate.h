#ifndef VALIDATE_H
#define VALIDATE_H

#include <stdint.h>

namespace Validate {
    /**
     * Check if a timestamp is a valid Unix epoch time
     * 
     * @param timestamp The timestamp to validate
     * @return true if timestamp is valid, false otherwise
     */
    bool validEpochTime(uint32_t timestamp);
}

#endif // VALIDATE_H