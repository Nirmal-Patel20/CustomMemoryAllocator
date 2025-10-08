#include "allocator/pool_allocator.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("Pool Allocator - allocation and deallocation speed(Pool vs Malloc)(64 bytes)",
          "[pool_allocator][comparison]") {

    const size_t OBJECT_SIZE = 64;
    const size_t NUM_OBJECTS = 5000;

    // Deallocation speed
    BENCHMARK_ADVANCED("Pool speed (Individual deallocation)")(
        Catch::Benchmark::Chronometer meter) {
        allocator::pool_allocator pool(OBJECT_SIZE, NUM_OBJECTS);

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(pool.allocate(OBJECT_SIZE));
            }

            for (auto ptr : ptrs) {
                pool.deallocate(ptr);
            }

            ptrs.clear();
        });
    };

    BENCHMARK_ADVANCED("Pool speed (Mass deallocation)")(Catch::Benchmark::Chronometer meter) {
        allocator::pool_allocator pool(OBJECT_SIZE, NUM_OBJECTS);

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(pool.allocate(OBJECT_SIZE));
            }

            pool.reset();
            ptrs.clear();
        });
    };

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

TEST_CASE("Pool allocator - Pool Growth Cost", "[pool_allocator][growthCost]") {

    BENCHMARK_ADVANCED("Growth-Performance")(Catch::Benchmark::Chronometer meter) {
        allocator::pool_allocator pool(64, 100, 8,
                                       10); // 64 byte objects, 100 per pool, max 1000 pools
        std::vector<void*> all_ptrs;
        all_ptrs.reserve(1000 * 10); // reserve for max capacity

        meter.measure([&] {
            // Force multiple pool allocations
            for (int pool_num = 0; pool_num < 10; ++pool_num) {
                // Fill current pool
                for (size_t i = 0; i < 100; ++i) {
                    all_ptrs.push_back(pool.allocate(64));
                }
                // Next allocation will trigger pool growth
            }

            // Cleanup
            pool.reset(); // back to initial state
            all_ptrs.clear();
        });

        // Cleanup
        pool.releaseMemory();
    };
}

TEST_CASE("Pool Allocator - Realistic Game Pattern", "[pool_allocator][gamePattern]") {

    struct Bullet {
        float position;
        float velocity;
        int damage;
        // other bullet data
    };

    BENCHMARK_ADVANCED("Game-Simulation")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            allocator::pool_allocator bullet_pool(sizeof(Bullet), 1000, alignof(Bullet));
            std::vector<void*> active_bullets;

            // Simulate 60 frames
            for (int frame = 0; frame < 60; ++frame) {
                // Spawn some bullets each frame
                for (int i = 0; i < 10; ++i) {
                    // Allocate a new bullet
                    Bullet* new_bullet = new (bullet_pool.allocate(sizeof(Bullet))) Bullet();

                    // Initialize bullet (simplified)
                    new_bullet->damage = 10;
                    new_bullet->position = 0.0f;

                    // Add to active bullets
                    active_bullets.push_back(new_bullet);
                }

                // Remove some bullets (simulate collision/timeout)
                if (active_bullets.size() > 50) {
                    for (int i = 0; i < 5; ++i) {
                        bullet_pool.deallocate(active_bullets.back());
                        active_bullets.pop_back();
                    }
                }
            }

            // clean up
            for (auto ptr : active_bullets) {
                bullet_pool.deallocate(ptr);
            }
        });
    };
}

TEST_CASE("Pool Allocator - Alignment Overhead", "[pool_allocator][alignmentOverhead]") {

    std::cerr << "--------------------------------------------------" << std::endl;
    std::cerr << "Running Pool Allocator - Alignment Overhead" << std::endl; // top-level code
    std::cerr << "--------------------------------------------------" << std::endl;

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

        SECTION(test.description) {

            std::cerr << "--------------------------------------------------" << std::endl;

            allocator::pool_allocator pool(test.raw_size, 100);

            // Get actual object size after alignment
            size_t actual_size = pool.getObjectSize();
            // Calculate overhead
            size_t padding = actual_size - test.raw_size;
            double overhead_percent = (double) padding / test.raw_size * 100.0;

            std::cout << "Raw size: " << test.raw_size << " bytes\n";
            std::cout << "Aligned size: " << actual_size << " bytes\n";
            std::cout << "Padding: " << padding << " bytes\n";
            std::cout << "Overhead: " << overhead_percent << "%\n\n";

            // Verify it matches expected
            REQUIRE(actual_size == test.expected_aligned_size);

            std::cerr << "--------------------------------------------------" << std::endl;
        }
    }
}