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

// Default alignment tests
TEST_CASE("stack_allocator - Default alignment(8 bytes)",
          "[stack_allocator][alignment]") { // Allocator use default alignment when user dont pass
                                            // alignment when allocate
    allocator::stack_allocator stackAllocator(128);

    void* ptr1 = stackAllocator.allocate(1); // 7 bytes padding so actual size 8 bytes
    REQUIRE(stackAllocator.getObjectSize() ==
            8); // getObjectSize can only give last allocating size

    void* ptr2 = stackAllocator.allocate(15); // 1 bytes padding so actual size 16 bytes
    REQUIRE(stackAllocator.getObjectSize() == 16);

    void* ptr3 = stackAllocator.allocate(32); // no padding
    REQUIRE(stackAllocator.getObjectSize() == 32);

    stackAllocator.releaseMemory();
}

TEST_CASE("stack_allocator - Pass default alignment", "[stack_allocator][alignment]") {
    // You can pass default alignment at construction of stack allocator and allocator will use that
    // but it must be between alignof(int) and alignof(max_align_t) which is usually 4 and 16
    allocator::stack_allocator stackAllocator(128, 4);

    void* ptr1 = stackAllocator.allocate(1); // 3 bytes padding so actual size 4 bytes
    REQUIRE(stackAllocator.getObjectSize() ==
            4); // getObjectSize can only give last allocating size

    void* ptr2 = stackAllocator.allocate(5); // 3 bytes padding so actual size 8 bytes
    REQUIRE(stackAllocator.getObjectSize() == 8);

    void* ptr3 = stackAllocator.allocate(15); // 1 bytes padding so actual size 16 bytes
    REQUIRE(stackAllocator.getObjectSize() == 16);

    void* ptr4 = stackAllocator.allocate(32); // no padding
    REQUIRE(stackAllocator.getObjectSize() == 32);

    stackAllocator.releaseMemory();
}

// Pass default alignment enforcements tests
// stack allocator first check it power of two then it between range

// Pass default alignment must be power of two
TEST_CASE("stack_allocator - Pass default alignment is power of two",
          "[stack_allocator][alignment]") {
    REQUIRE_NOTHROW(allocator::stack_allocator(125, 16));
}

TEST_CASE("stack_allocator - Pass default alignment is non-power of two",
          "[stack_allocator][alignment]") {
    REQUIRE_THROWS_AS(allocator::stack_allocator(125, 5), std::invalid_argument);
}

// Passed default alignment must be between alignof(int) and alignof(max_align_t) which is usually 4
// and 16 bytes
TEST_CASE("stack_allocator - Pass default alignment is less than alignof(int) usually 4 byes",
          "[stack_allocator][alignment]") {
    REQUIRE_THROWS_AS(allocator::stack_allocator(125, 2), std::invalid_argument);
}

TEST_CASE(
    "stack_allocator - Pass default alignment is more than alignof(max_align_t) usually 16 byes",
    "[stack_allocator][alignment]") {
    REQUIRE_THROWS_AS(allocator::stack_allocator(125, 32), std::invalid_argument);
}

// Alignment tests when we allocate
TEST_CASE("stack_allocator - allocate alignment", "[stack_allocator][alignment]") {
    allocator::stack_allocator stackAllocator(128); // 8 bytes default alignment

    void* ptr1 = stackAllocator.allocate(
        1, 4); // now it will get align to passed alignment not to default one
    REQUIRE(stackAllocator.getObjectSize() == 4);

    void* ptr2 = stackAllocator.allocate(1); // align to default alignment
    REQUIRE(stackAllocator.getObjectSize() == 8);

    void* ptr3 =
        stackAllocator.allocate(1, 16); // 16 bytes alignment so 15 bytes padding so actual siz 16
    REQUIRE(stackAllocator.getObjectSize() == 16);

    void* ptr4 =
        stackAllocator.allocate(16, 32); // 32 bytes alignment so 16 bytes padding so actual siz 32
    REQUIRE(stackAllocator.getObjectSize() == 32);

    stackAllocator.releaseMemory();
}

// Alignment at allocation time enforcements tests
// stack allocator first check it power of two then it between range

// Pass alignment must be power of two
TEST_CASE("stack_allocator - Pass alignment is power of two", "[stack_allocator][alignment]") {
    allocator::stack_allocator stackAllocator(128, 8);
    REQUIRE_NOTHROW(stackAllocator.allocate(1, 16));
}

TEST_CASE("stack_allocator - Pass alignment is non-power of two", "[stack_allocator][alignment]") {
    allocator::stack_allocator stackAllocator(128, 8);
    REQUIRE_THROWS_AS(stackAllocator.allocate(1, 15), std::invalid_argument);
}

// Unlike default alignment, we can passed alignment any alignment if it greater than and equal to
// alignof(int) which is usually 4 and it is power of two
TEST_CASE("stack_allocator - Pass alignment is less than alignof(int) usually 4 byes",
          "[stack_allocator][alignment]") {
    allocator::stack_allocator stackAllocator(128, 8);
    REQUIRE_THROWS_AS(stackAllocator.allocate(6, 1), std::invalid_argument);
}

// Be caution the higher the alignment, the higher chances of internal fragmentation
TEST_CASE("stack_allocator - Pass alignment is more than alignof(max_align_t) usually 16 byes",
          "[stack_allocator][alignment]") {
    allocator::stack_allocator stackAllocator(128, 8);

    REQUIRE_NOTHROW(stackAllocator.allocate(1, 128));

    REQUIRE(stackAllocator.getObjectSize() == 128); // 127 padding, too much internal fragmentation
}