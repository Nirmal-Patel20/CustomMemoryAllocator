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

namespace allocatorChecks {

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
} // namespace allocatorChecks

#endif // ALLOCATORUTILITIES_HPP