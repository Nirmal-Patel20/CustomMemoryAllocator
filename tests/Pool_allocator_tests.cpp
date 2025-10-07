#include "allocator/pool_allocator.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

// Allocate a block
TEST_CASE("Pool Allocator - Allocate and deallocate blocks", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    // Deallocate the block
    poolAllocator.deallocate(ptr1);
}

// Allocate more than one block
TEST_CASE("Pool Allocator - Allocate multiple blocks", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = poolAllocator.allocate(32);
    REQUIRE(ptr2 != nullptr);

    // Deallocate the blocks
    poolAllocator.deallocate(ptr1);
    poolAllocator.deallocate(ptr2);
}

// Attempt to allocate a block larger than the block size
TEST_CASE("Pool Allocator - Allocate block larger than block size", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    REQUIRE_THROWS_AS(poolAllocator.allocate(64), std::runtime_error);
}

// Allocate again, should reuse the freed block
TEST_CASE("Pool Allocator - Reuse freed blocks", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    poolAllocator.deallocate(ptr1);

    void* ptr2 = poolAllocator.allocate(16);
    REQUIRE(ptr2 == ptr1); // Should reuse the same block

    poolAllocator.deallocate(ptr2);
}

// Check allocated size (should be equal to pool size)
TEST_CASE("Pool Allocator - Check allocated size", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    size_t allocatedSize = poolAllocator.getAllocatedSize();
    REQUIRE(allocatedSize == 0); // All blocks should be freed
}

// Reset the allocator and check allocated size again
TEST_CASE("Pool Allocator - Reset pool allocator", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);
    void* ptr2 = poolAllocator.allocate(16);
    REQUIRE(ptr2 != nullptr);

    poolAllocator.reset();
    size_t allocatedSize = poolAllocator.getAllocatedSize();

    REQUIRE(allocatedSize == 0); // All blocks should be freed after reset
}

// release memory
TEST_CASE("Pool Allocator - Release memory and try allocating again", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    for (int i = 0; i < 15; i++) {
        void* ptr = poolAllocator.allocate(16);
        REQUIRE(ptr != nullptr);
    }

    poolAllocator.releaseMemory(); // no longer owns memory, cannot allocate more and this pool is
                                   // unusable unless we call reset()

    REQUIRE_THROWS_AS(poolAllocator.allocate(16),
                      std::runtime_error); // Should throw since memory is released
}

TEST_CASE("Pool Allocator - check grow beyond initial pool", "[pool_allocator][basic]") {
    allocator::pool_allocator smallPool(32, 2, alignof(max_align_t),
                                        2); // Small pool with 2 pools max

    void* ptr1 = smallPool.allocate(16);
    REQUIRE(ptr1 != nullptr);

    void* ptr2 = smallPool.allocate(16);
    REQUIRE(ptr2 != nullptr);

    // Pool is full now, next allocation should trigger growth for the first time
    void* ptr3 = smallPool.allocate(16);
    REQUIRE(ptr3 != nullptr);

    // Next allocation should also succeed uses the second growth
    void* ptr4 = smallPool.allocate(16);
    REQUIRE(ptr4 != nullptr);

    // Next allocation should fail (exceeds max grow count)
    REQUIRE_THROWS_AS(smallPool.allocate(16), std::runtime_error);

    // Clean up
    smallPool.releaseMemory();
}

TEST_CASE("Pool Allocator - try allocating more than max capacity(64 MB)",
          "[pool_allocator][basic]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(32, 65ull * 1024 * 1024), std::invalid_argument);
}

// Alignment must be power of two
TEST_CASE("Pool Allocator - Non power of two alignment", "[pool_allocator][alignment]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 5),
                      std::invalid_argument); // alignment must be power of two
}

TEST_CASE("Pool Allocator - Power of two alignment", "[pool_allocator][alignment]") {
    REQUIRE_NOTHROW(allocator::pool_allocator(16, 32, 8)); // alignment must be power of two
}

TEST_CASE("Pool Allocator - alignment must be between alignof(int) and alignof(max_align_t)",
          "[alignment]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 3), std::invalid_argument);
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 20), std::invalid_argument);
}