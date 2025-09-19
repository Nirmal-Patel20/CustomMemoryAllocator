#include "allocator/stack_allocator.hpp"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

// Allocate a chunk without default alignment (8 bytes)
TEST_CASE("stack_allocator - Allocate a chunk on the stack allocator", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(
        32); // 8 bytes default alignment if not passed when allocating
    void* ptr1 = stackAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    // Deallocate the chunk
    stackAllocator.deallocate(ptr1);
}

// Allocate a chunk with default alignment
TEST_CASE("stack_allocator - Allocate a chunk on the stack allocator with default alignment",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(
        32, 4); // 4 bytes default alignment if not passed when allocating
    void* ptr1 = stackAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    // Deallocate the chunk
    stackAllocator.deallocate(ptr1);
}

// Allocate more than one chunk
TEST_CASE("stack_allocator - Allocate multiple chunks", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(100, 4);
    void* ptr1 = stackAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = stackAllocator.allocate(32);
    REQUIRE(ptr2 != nullptr);

    // Deallocate the chunks (LIFO)
    stackAllocator.deallocate(ptr2);
    stackAllocator.deallocate(ptr1);
}

// Allocate again, should reuse the freed chunk
TEST_CASE("stack_allocator - Reuse freed chunk", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(128, 8);
    void* ptr1 = stackAllocator.allocate(50);
    REQUIRE(ptr1 != nullptr);

    stackAllocator.deallocate(ptr1);

    void* ptr2 = stackAllocator.allocate(30);
    REQUIRE(ptr2 == ptr1); // Should reuse the same chunk

    stackAllocator.deallocate(ptr2);
}

// Check allocated size (should be equal to 0)
TEST_CASE("stack_allocator - Check allocated size", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(320, 8);
    size_t allocatedSize = stackAllocator.getAllocatedSize();
    REQUIRE(allocatedSize == 0); // All chunks should be freed
}

// Reset the allocator and check allocated size again
TEST_CASE("stack_allocator - Reset stack allocator", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(32, 8);
    void* ptr1 = stackAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = stackAllocator.allocate(16);
    REQUIRE(ptr2 != nullptr);

    stackAllocator.reset();
    size_t allocatedSize = stackAllocator.getAllocatedSize();

    REQUIRE(allocatedSize == 0); // All chunks should be freed after reset
}

// release memory
TEST_CASE("stack_allocator - Release memory and try allocating again", "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(1200, 8);
    for (int i = 0; i < 15; i++) {
        void* ptr = stackAllocator.allocate(16);
        REQUIRE(ptr != nullptr);
    }

    stackAllocator.releaseMemory(); // no longer owns memory, cannot allocate more and this stack is
                                    // unusable unless we call reset()

    REQUIRE_THROWS_AS(stackAllocator.allocate(16),
                      std::runtime_error); // Should throw since memory is released
}

TEST_CASE("stack_allocator - try allocating more than max capacity(64 MB)",
          "[stack_allocator][basic]") {
    REQUIRE_THROWS_AS(allocator::stack_allocator(65ull * 1024 * 1024, 16), std::invalid_argument);
}

// Attempt to allocate a chunk larger than the buffer
TEST_CASE("stack_allocator - Allocate chunk larger than buffer size on non-resizable allocator",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(64, 8);
    REQUIRE_THROWS_AS(stackAllocator.allocate(75), std::runtime_error);
}

// Attempt to allocate multiple chunks exceeding buffer size on non-resizable allocator
TEST_CASE(
    "stack_allocator - Allocate multiple chunks exceeding buffer size on non-resizable allocator",
    "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(64, 8);
    void* ptr1 = stackAllocator.allocate(30); // will align to 8 bytes so 32
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = stackAllocator.allocate(30);
    REQUIRE(ptr2 != nullptr);
    REQUIRE_THROWS_AS(stackAllocator.allocate(10), std::runtime_error);
}

// Attempt to allocate multiple chunks exceeding buffer size on non-resizable allocator
TEST_CASE("stack_allocator - Allocate multiple chunks exceeding buffer size on resizable allocator",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stackAllocator(64, 8, true);
    void* ptr1 = stackAllocator.allocate(30); // will align to 8 bytes so 32
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = stackAllocator.allocate(30);
    REQUIRE(ptr2 != nullptr);
    REQUIRE_NOTHROW(stackAllocator.allocate(10)); // will create new buffer same size as previous
}