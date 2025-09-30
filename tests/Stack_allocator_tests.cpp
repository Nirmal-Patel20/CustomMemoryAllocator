#include "allocator/stack_allocator.hpp"
#include <algorithm>
#include <array>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
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

// Resizable and non-resizable stack allocator

// By default stack allocator is non-resizable means that it can not exceed it size
TEST_CASE("stack_allocator - By default stack allocator is non-resizable",
          "[stack_allocator][basic]") {
    allocator::stack_allocator small_stack(32);
    void* ptr1 = small_stack.allocate(16);
    void* ptr2 = small_stack.allocate(16);

    // Now stack has no allocation chunks so next allocation will fail
    REQUIRE_THROWS_AS(small_stack.allocate(16), std::runtime_error);
}

// You can even set stack allocator as resizable by passing 3rd argument to constructor
TEST_CASE("stack_allocator - set stack allocator as resizable", "[stack_allocator][basic]") {
    allocator::stack_allocator small_stack(32, 8, true);
    void* ptr1 = small_stack.allocate(16);
    void* ptr2 = small_stack.allocate(16);

    // Now stack has no allocation chunks so next allocation will fail
    REQUIRE_NOTHROW(small_stack.allocate(16));
}

// Basic mark functionality
TEST_CASE("stack_allocator - basic mark usage", "[stack_allocator][basic]") {
    allocator::stack_allocator stack(256, 8, true);

    void* ptr1 = stack.allocate(16);
    void* ptr2 = stack.allocate(16);

    REQUIRE(stack.getAllocatedSize() == 32);

    // Take a mark after 32 bytes have been allocated
    auto mark = stack.mark();

    void* ptr3 = stack.allocate(32);
    void* ptr4 = stack.allocate(64);

    REQUIRE(stack.getAllocatedSize() == 128);

    // Reset allocator to the mark; allocated size should revert to 32
    stack.reset_to_mark(mark);

    REQUIRE(stack.getAllocatedSize() == 32);
}

// Allocator shrinks back when new buffers were added after mark
TEST_CASE("stack_allocator - reset_to_mark shrinks buffers to mark", "[stack_allocator][basic]") {
    allocator::stack_allocator stack(32, 8, true);

    void* ptr1 = stack.allocate(16);
    void* ptr2 = stack.allocate(16);

    // Stack is full now
    REQUIRE(stack.getAllocatedSize() == 32);

    auto mark = stack.mark();

    // Allocate 2 more buffers
    void* ptr3 = stack.allocate(32);
    void* ptr4 = stack.allocate(32);

    REQUIRE(stack.getAllocatedSize() == 96);

    // Reset allocator to the mark; buffers should shrink
    stack.reset_to_mark(mark);

    REQUIRE(stack.getAllocatedSize() == 32);
}

// Reset to mark throws if allocator's offset is less than mark offset
TEST_CASE("stack_allocator - reset_to_mark throws if current offset is less than mark",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stack(256, 8, true);

    void* ptr1 = stack.allocate(16);
    void* ptr2 = stack.allocate(16);

    REQUIRE(stack.getAllocatedSize() == 32);

    auto mark = stack.mark();

    stack.deallocate(ptr2); // Now allocator offset is 16; mark offset is 32

    REQUIRE_THROWS_AS(stack.reset_to_mark(mark), std::runtime_error);
}

// Reset throws if all buffers were released after mark
TEST_CASE("stack_allocator - reset_to_mark throws if no buffers exist",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stack(256, 8, true);

    void* ptr1 = stack.allocate(16);
    void* ptr2 = stack.allocate(16);

    REQUIRE(stack.getAllocatedSize() == 32);

    auto mark = stack.mark(); // Mark after 32 bytes

    void* ptr3 = stack.allocate(32);
    void* ptr4 = stack.allocate(64);

    stack.releaseMemory();

    // Reset should throw because no buffers exist
    REQUIRE_THROWS_AS(stack.reset_to_mark(mark), std::invalid_argument);
}

// Reset throws if allocator has fewer buffers than the mark (negative sizeDiff)
TEST_CASE("stack_allocator - reset_to_mark throws on negative sizeDiff",
          "[stack_allocator][basic]") {
    allocator::stack_allocator stack(32, 8, true);

    void* ptr1 = stack.allocate(16);
    void* ptr2 = stack.allocate(16);
    void* ptr3 = stack.allocate(16);
    void* ptr4 = stack.allocate(16);

    REQUIRE(stack.getAllocatedSize() == 64);

    // Take a mark when 2 buffer exists
    auto mark = stack.mark();

    stack.deallocate(ptr4);
    stack.deallocate(ptr3); // now buffers.size() < mark size

    // Reset should throw because current buffer count < mark buffer count
    REQUIRE_THROWS_AS(stack.reset_to_mark(mark), std::runtime_error);
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

// benchmarks
TEST_CASE("stack Allocator - allocating speed(stack vs Malloc)(64 bytes)",
          "[stack_allocator][benchmark][comparison]") {

    allocator::DebugGuard no_debug(false);
    allocator::CapacityGuard no_capacity_checks(false);

    const size_t OBJECT_SIZE = 64;
    const size_t NUM_OBJECTS = 5000;
    const size_t STACK_SIZE = OBJECT_SIZE * NUM_OBJECTS;

    // Test malloc
    // Allocation speed
    BENCHMARK_ADVANCED("stack allocating speed")(Catch::Benchmark::Chronometer meter) {

        allocator::stack_allocator stack(STACK_SIZE, 8, true); // resizable for benchmark
        std::vector<void*> ptrs;

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(stack.allocate(OBJECT_SIZE));
            }
        });

        stack.releaseMemory();
    };

    BENCHMARK_ADVANCED("Malloc allocating speed")(Catch::Benchmark::Chronometer meter) {

        std::vector<void*> ptrs;

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(malloc(OBJECT_SIZE));
            }
        });

        for (auto ptr : ptrs) {
            free(ptr);
        }
    };
}

TEST_CASE("stack Allocator - allocation and deallocation speed(stack vs Malloc)(64 bytes)",
          "[stack_allocator][benchmark][comparison]") {

    const size_t OBJECT_SIZE = 64;
    const size_t NUM_OBJECTS = 5000;
    const size_t STACK_SIZE = OBJECT_SIZE * NUM_OBJECTS;

    allocator::DebugGuard no_debug(false);
    allocator::CapacityGuard no_capacity_checks(false);

    // Test stack
    BENCHMARK_ADVANCED("stack speed (LIFO deallocation)")(Catch::Benchmark::Chronometer meter) {

        allocator::stack_allocator stack(STACK_SIZE, 8);

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(stack.allocate(OBJECT_SIZE));
            }

            while (!ptrs.empty()) {
                stack.deallocate(ptrs.back());
                ptrs.pop_back();
            }
        });
    };

    BENCHMARK_ADVANCED("stack speed (mass deallocation)")(Catch::Benchmark::Chronometer meter) {

        allocator::stack_allocator stack(STACK_SIZE, 8);

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(stack.allocate(OBJECT_SIZE));
            }

            stack.reset();
        });
    };

    // Test malloc
    BENCHMARK_ADVANCED("Malloc speed")(Catch::Benchmark::Chronometer meter) {

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(malloc(OBJECT_SIZE));
            }

            for (auto ptr : ptrs) {
                free(ptr);
            }

            ptrs.clear();
        });
    };
}

TEST_CASE("stack allocator - stack Growth Cost", "[stack_allocator][benchmark][growthCost]") {

    BENCHMARK_ADVANCED("Growth-Performance")(Catch::Benchmark::Chronometer meter) {
        allocator::stack_allocator stack(640, 8, true);
        std::vector<void*> all_ptrs;

        meter.measure([&] {
            // Force multiple stack allocations
            for (int stack_num = 0; stack_num < 10; ++stack_num) {
                // Fill current stack
                for (size_t i = 0; i < 100; ++i) {
                    all_ptrs.push_back(stack.allocate(64));
                }
                // Next allocation will trigger stack growth
            }
        });

        // Cleanup
        stack.releaseMemory();
    };
}

TEST_CASE("stack Allocator - Realistic Game Pattern", "[stack_allocator][benchmark][gamePattern]") {

    allocator::stack_allocator frame_stack(1 * 1024 * 1024); // 1MB frame budget

    struct Vector3 {
        std::array<float, 3> data;

        Vector3(float x, float y, float z) : data{{x, y, z}} {}
    };

    struct Matrix4 {
        std::array<float, 16> data; // column-major or row-major
    };

    allocator::DebugGuard(false); // Simulate release mode

    BENCHMARK_ADVANCED("Game-Simulation")(Catch::Benchmark::Chronometer meter) {

        meter.measure([&] {
            // Simulate 60 frames
            for (int frame = 0; frame < 60; ++frame) {
                // AI calculations (temporary arrays)
                float* ai_distances =
                    static_cast<float*>(frame_stack.allocate(100 * sizeof(float)));
                Vector3* ai_paths =
                    static_cast<Vector3*>(frame_stack.allocate(100 * sizeof(Vector3)));

                // Rendering temp data
                Matrix4* temp_matrices =
                    static_cast<Matrix4*>(frame_stack.allocate(200 * sizeof(Matrix4)));
                float* depth_values =
                    static_cast<float*>(frame_stack.allocate(200 * sizeof(float)));

                // UI string formatting
                char* temp_strings =
                    static_cast<char*>(frame_stack.allocate(1024)); // Various UI strings

                // Audio mixing buffers
                float* audio_samples =
                    static_cast<float*>(frame_stack.allocate(4096 * sizeof(float)));

                // Simulate using the data (prevent optimization)
                volatile float sum = 0;
                for (int i = 0; i < 100; ++i) {
                    sum += ai_distances[i];
                }

                frame_stack.reset();
            }
        });
    };

    allocator::g_debug_checks = true;
}

TEST_CASE("stack Allocator - Alignment Overhead",
          "[stack_allocator][benchmark][alignmentOverhead]") {

    // Test different object sizes to show alignment impact
    struct TestData {
        size_t raw_size;
        size_t expected_aligned_size;
        const char* description;
    };

    std::vector<TestData> test_cases = {{1, 8, "Single byte -> 8 bytes (7 bytes padding)"},
                                        {17, 24, "17 bytes -> 24 bytes (7 bytes padding)"},
                                        {33, 40, "33 bytes -> 40 bytes (7 bytes padding)"},
                                        {65, 72, "65 bytes -> 72 bytes (7 bytes padding)"},
                                        {64, 64, "64 bytes -> 64 bytes (perfect fit)"},
                                        {128, 128, "128 bytes -> 128 bytes (perfect fit)"}};

    for (const auto& test : test_cases) {

        allocator::stack_allocator stack(1024);

        SECTION(test.description) {

            // Get actual object size after alignment
            stack.allocate(test.raw_size);
            size_t actual_size = stack.getObjectSize();
            // Calculate overhead
            size_t padding = actual_size - test.raw_size;
            double overhead_percent = (double) padding / test.raw_size * 100.0;

            std::cout << "Raw size: " << test.raw_size << " bytes\n";
            std::cout << "Aligned size: " << actual_size << " bytes\n";
            std::cout << "Padding: " << padding << " bytes\n";
            std::cout << "Overhead: " << overhead_percent << "%\n\n";

            // Verify it matches expected
            REQUIRE(actual_size == test.expected_aligned_size);
        }
    }
}