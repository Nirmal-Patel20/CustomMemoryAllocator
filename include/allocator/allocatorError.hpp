#ifndef ALLOCATORERROR_HPP
#define ALLOCATORERROR_HPP

#include <stdexcept>
#include <string>

// make error handling configurable. In debug mode, throw detailed exceptions.
// In release mode, throw std::bad_alloc for allocation failures.
namespace allocator {

inline void throwAllocationError([[maybe_unused]] std::string allocationType,
                                 [[maybe_unused]] std::string message) {
#if defined(ALLOCATOR_DEBUG)
    throw std::runtime_error("Allocation Error in " + allocationType + ": " + message);
#else
    throw std::bad_alloc();
#endif
}
} // namespace allocator

#endif // ALLOCATORERROR_HPP