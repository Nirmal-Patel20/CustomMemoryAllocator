#ifndef ALLOCATORUTILITIES_HPP
#define ALLOCATORUTILITIES_HPP

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>

// Guarantee ALLOCATOR_DEBUG is always defined
#ifndef ALLOCATOR_DEBUG
#ifdef NDEBUG
#define ALLOCATOR_DEBUG 0
#else
#define ALLOCATOR_DEBUG 1
#endif
#endif

namespace allocator {

// make error handling configurable. In debug mode, throw detailed exceptions.
// In release mode, throw std::bad_alloc for allocation failures.
inline void throwAllocationError([[maybe_unused]] std::string allocationType,
                                 [[maybe_unused]] std::string message) {
#if ALLOCATOR_DEBUG
    throw std::runtime_error("Allocation Error in " + allocationType + ": " + message);
#else
    throw std::bad_alloc();
#endif
}

// Global flag controlling debug checks and Capacity_checks
#if ALLOCATOR_DEBUG
inline std::atomic<bool> g_debug_checks = true;
#else
inline constexpr std::atomic<bool> g_debug_checks = false;
#endif

#if ALLOCATOR_DEBUG
inline std::atomic<bool> Max_Capacity_checks = true;
#else
inline constexpr std::atomic<bool> Max_Capacity_checks = true;
#endif

} // namespace allocator

#endif // ALLOCATORUTILITIES_HPP