#include "allocator/buddy_allocator.hpp"
#include <catch2/catch_test_macros.hpp>

// Allocate a block
TEST_CASE("buddy Allocator - Allocate and deallocate blocks", "[buddy_allocator][basic]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(1024);             // 1kb block (minimum size)
    REQUIRE(ptr1 != nullptr);

    // Deallocate the block
    buddyAllocator.deallocate(ptr1);
}

// Allocate multiple blocks
TEST_CASE("buddy Allocator - Allocate multiple blocks", "[buddy_allocator][multiple]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(2048);             // 2kb block
    void* ptr2 = buddyAllocator.allocate(4096);             // 4kb block
    void* ptr3 = buddyAllocator.allocate(8192);             // 8kb block

    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(ptr3 != nullptr);

    // Deallocate the blocks
    buddyAllocator.deallocate(ptr1);
    buddyAllocator.deallocate(ptr2);
    buddyAllocator.deallocate(ptr3);

    REQUIRE(buddyAllocator.getAllocatedSize() == 0);
}

// Merging buddies after deallocation
TEST_CASE("buddy Allocator - Merge buddies after deallocation", "[buddy_allocator][basic]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(2048);             // 2kb block
    void* ptr2 = buddyAllocator.allocate(2048);             // 2kb block

    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);

    // Deallocate the blocks
    buddyAllocator.deallocate(ptr1);
    buddyAllocator.deallocate(ptr2);

    // every deallocation should try to merge buddies and free up larger blocks if both buddies are
    // free currently we can't directly test internal state, but i can ensure no memory leaks

    REQUIRE(buddyAllocator.getAllocatedSize() == 0);

    // right now free lists should contain one large block, which is the initial block as no other
    // allocations were made and allocated size is zero you run this test in debug mode to check for
    // memory leaks or free list integrity
}

// Minimum and maximum allocation sizes
TEST_CASE("buddy Allocator - Minimum and maximum allocation sizes", "[buddy_allocator][edge]") {
    SECTION("Minimum size allocation") {
        REQUIRE_THROWS_AS(allocator::buddy_allocator(512), std::invalid_argument); // less than 1KB
    }

    SECTION("Maximum size allocation") {
        REQUIRE_THROWS_AS(allocator::buddy_allocator(256ull * 1024 * 1024),
                          std::invalid_argument); // more than 128MB
    }
}

// minimum allocation and maximum allocation(buffer size)
TEST_CASE("buddy Allocator - Allocate minimum and maximum sizes", "[buddy_allocator][edge]") {
    allocator::buddy_allocator buddyAllocator(4ull * 1024 * 1024); // 4MB buffer

    // Minimum allocation
    REQUIRE_THROWS(buddyAllocator.allocate(512)); // less than 1KB should throw

    // Maximum allocation
    REQUIRE_THROWS(
        buddyAllocator.allocate(8ull * 1024 * 1024)); // more than buffer size should throw
}

// release memory
TEST_CASE("buddy Allocator - Release memory", "[buddy_allocator][basic]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(2048);             // 2kb block

    REQUIRE(ptr1 != nullptr);

    buddyAllocator.releaseMemory();

    // After releasing memory, allocation should fail
    REQUIRE_THROWS_AS(buddyAllocator.allocate(2048),
                      std::runtime_error); // or in release mode return nullptr
}

// Reset allocator
TEST_CASE("buddy Allocator - Reset allocator", "[buddy_allocator][basic]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(2048);             // 2kb block
    void* ptr2 = buddyAllocator.allocate(4096);             // 4kb block

    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);

    buddyAllocator.reset();

    REQUIRE(buddyAllocator.getAllocatedSize() == 0);

    // After reset, should be able to allocate again
    void* ptr3 = buddyAllocator.allocate(2048); // 2kb block
    REQUIRE(ptr3 != nullptr);

    buddyAllocator.releaseMemory();

    // calling reset after release should reallocate memory
    buddyAllocator.reset();

    REQUIRE_NOTHROW(buddyAllocator.allocate(2048)); // 2kb block
}

// Invalid deallocation(e.g., double free, invalid pointer, null pointer, after release)(edge cases)
TEST_CASE("buddy Allocator - Invalid deallocation", "[buddy_allocator][edge]") {
    allocator::buddy_allocator buddyAllocator(1024 * 1024); // 1mb buffer
    void* ptr1 = buddyAllocator.allocate(2048);             // 2kb block

    REQUIRE(ptr1 != nullptr);

    // Double free
    buddyAllocator.deallocate(ptr1);
    REQUIRE_THROWS_AS(buddyAllocator.deallocate(ptr1), std::invalid_argument);

    // Invalid pointer
    int invalidPtr;
    REQUIRE_THROWS_AS(buddyAllocator.deallocate(&invalidPtr), std::invalid_argument);

    // Null pointer
    REQUIRE_THROWS_AS(buddyAllocator.deallocate(nullptr), std::invalid_argument);

    // Deallocation after release
    buddyAllocator.releaseMemory();
    REQUIRE_THROWS_AS(buddyAllocator.deallocate(ptr1), std::invalid_argument);
}
