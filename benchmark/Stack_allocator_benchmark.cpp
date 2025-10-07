#include "allocator/stack_allocator.hpp"
#include <array>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("stack Allocator - allocating speed(stack vs Malloc)(64 bytes)",
          "[stack_allocator][benchmark][comparison]") {

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

    BENCHMARK_ADVANCED("Game-Simulation")(Catch::Benchmark::Chronometer meter) {

        meter.measure([&] {
            // Simulate 60 frames
            for (int frame = 0; frame < 60; ++frame) {
                // AI calculations (temporary arrays)
                float* ai_distances =
                    static_cast<float*>(frame_stack.allocate(100 * sizeof(float)));
                [[maybe_unused]] Vector3* ai_paths =
                    static_cast<Vector3*>(frame_stack.allocate(100 * sizeof(Vector3)));

                // Rendering temp data
                [[maybe_unused]] Matrix4* temp_matrices =
                    static_cast<Matrix4*>(frame_stack.allocate(200 * sizeof(Matrix4)));
                [[maybe_unused]] float* depth_values =
                    static_cast<float*>(frame_stack.allocate(200 * sizeof(float)));

                // UI string formatting
                [[maybe_unused]] char* temp_strings =
                    static_cast<char*>(frame_stack.allocate(1024)); // Various UI strings

                // Audio mixing buffers
                [[maybe_unused]] float* audio_samples =
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
            [[maybe_unused]] void* ptr = stack.allocate(test.raw_size);
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