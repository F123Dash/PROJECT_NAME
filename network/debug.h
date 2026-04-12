#ifndef DEBUG_H
#define DEBUG_H

#include <stdexcept>
#include <string>

// Global debug flag - controls verbose logging
extern bool DEBUG_MODE;

// Runtime assertion with error message
inline void ASSERT(bool cond, const std::string& msg) {
    if (!cond) {
        throw std::runtime_error("[ASSERT FAILED] " + msg);
    }
}

// Debug assertion (only checked if DEBUG_MODE is true)
inline void DEBUG_ASSERT(bool cond, const std::string& msg) {
    if (DEBUG_MODE && !cond) {
        throw std::runtime_error("[DEBUG_ASSERT FAILED] " + msg);
    }
}

#endif