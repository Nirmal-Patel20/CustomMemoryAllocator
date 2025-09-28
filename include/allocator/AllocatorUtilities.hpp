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

// Global debug flags
#if ALLOCATOR_DEBUG
inline std::atomic<bool> g_debug_checks{true};
inline std::atomic<bool> g_capacity_checks{true};

// RAII guards - only exist in debug mode
class DebugGuard {
  private:
    bool old_value;

  public:
    explicit DebugGuard(bool new_value) : old_value(g_debug_checks.load()) {
        g_debug_checks.store(new_value);
    }

    ~DebugGuard() { g_debug_checks.store(old_value); }

    DebugGuard(const DebugGuard&) = delete;
    DebugGuard& operator=(const DebugGuard&) = delete;
};

class CapacityGuard {
  private:
    bool old_value;

  public:
    explicit CapacityGuard(bool new_value) : old_value(g_capacity_checks.load()) {
        g_capacity_checks.store(new_value);
    }

    ~CapacityGuard() { g_capacity_checks.store(old_value); }

    CapacityGuard(const CapacityGuard&) = delete;
    CapacityGuard& operator=(const CapacityGuard&) = delete;
};

#else
// Release mode: compile-time constants, no guards needed
inline constexpr bool g_debug_checks = false;
inline constexpr bool g_capacity_checks = true;

// No guard classes in release mode
#endif
} // namespace allocator

#endif // ALLOCATORUTILITIES_HPP