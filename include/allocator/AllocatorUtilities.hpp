#ifndef ALLOCATORUTILITIES_HPP
#define ALLOCATORUTILITIES_HPP

#include <stdexcept>
#include <string>
#include <atomic>

namespace allocator {

// make error handling configurable. In debug mode, throw detailed exceptions.
// In release mode, throw std::bad_alloc for allocation failures.
inline void throwAllocationError([[maybe_unused]] std::string allocationType,
                                 [[maybe_unused]] std::string message) {
    #if defined(ALLOCATOR_DEBUG)
        throw std::runtime_error("Allocation Error in " + allocationType + ": " + message);
    #else
        throw std::bad_alloc();
    #endif
}

// Global flag controlling debug checks
inline std::atomic<bool> g_debug_checks{true};

} // namespace allocator

#endif // ALLOCATORUTILITIES_HPP