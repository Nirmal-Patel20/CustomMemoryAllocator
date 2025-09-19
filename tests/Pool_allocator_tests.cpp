#include "allocator/pool_allocator.hpp"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

// Allocate a block
TEST_CASE("Allocate and deallocate blocks", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    // Deallocate the block
    poolAllocator.deallocate(ptr1);
}

// Allocate more than one block
TEST_CASE("Allocate multiple blocks", "[pool_allocator][basic]") {
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
TEST_CASE("Allocate block larger than block size", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    REQUIRE_THROWS_AS(poolAllocator.allocate(64), std::runtime_error);
}

// Allocate again, should reuse the freed block
TEST_CASE("Reuse freed blocks", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    void* ptr1 = poolAllocator.allocate(16);
    REQUIRE(ptr1 != nullptr);

    poolAllocator.deallocate(ptr1);

    void* ptr2 = poolAllocator.allocate(16);
    REQUIRE(ptr2 == ptr1); // Should reuse the same block

    poolAllocator.deallocate(ptr2);
}

// Check allocated size (should be equal to pool size)
TEST_CASE("Check allocated size", "[pool_allocator][basic]") {
    allocator::pool_allocator poolAllocator(32, 1000);
    size_t allocatedSize = poolAllocator.getAllocatedSize();
    REQUIRE(allocatedSize == 0); // All blocks should be freed
}

// Reset the allocator and check allocated size again
TEST_CASE("Reset pool allocator", "[pool_allocator][basic]") {
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
TEST_CASE("Release memory and try allocating again", "[pool_allocator][basic]") {
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

TEST_CASE("check grow beyond initial pool", "[pool_allocator][basic]") {
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

TEST_CASE("try allocating more than max capacity(64 MB)", "[pool_allocator][basic]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(32, 65ull * 1024 * 1024), std::invalid_argument);
}

// Alignment must be power of two
TEST_CASE("Non power of two alignment", "[pool_allocator][alignment]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 5),
                      std::invalid_argument); // alignment must be power of two
}

TEST_CASE("Power of two alignment", "[pool_allocator][alignment]") {
    REQUIRE_NOTHROW(allocator::pool_allocator(16, 32, 8)); // alignment must be power of two
}

TEST_CASE("alignment must be between alignof(int) and alignof(max_align_t)", "[alignment]") {
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 3), std::invalid_argument);
    REQUIRE_THROWS_AS(allocator::pool_allocator(16, 32, 20), std::invalid_argument);
}

// benchmarks
TEST_CASE("Pool Allocator - Pool vs Malloc (64 byte objects)",
          "[pool_allocator][benchmark][comparison]") {

    const size_t OBJECT_SIZE = 64;
    const size_t NUM_OBJECTS = 10000;

    // Test malloc
    BENCHMARK_ADVANCED("Malloc")(Catch::Benchmark::Chronometer meter) {
        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(malloc(OBJECT_SIZE));
            }
        });

        for (auto ptr : ptrs) {
            free(ptr);
        }
    };

    // Test pool
    BENCHMARK_ADVANCED("pool")(Catch::Benchmark::Chronometer meter) {
        allocator::pool_allocator pool(OBJECT_SIZE, NUM_OBJECTS);

        std::vector<void*> ptrs;
        ptrs.reserve(NUM_OBJECTS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_OBJECTS; ++i) {
                ptrs.push_back(pool.allocate(OBJECT_SIZE));
            }
        });

        for (auto& ptr : ptrs) {
            pool.deallocate(ptr);
        }
    };
}

TEST_CASE("Pool Allocator - pool allocating speed", "[pool_allocator][benchmark][speed]") {
    allocator::pool_allocator pool(64, 1000);

    BENCHMARK_ADVANCED("Pool-64byte-alloc")(Catch::Benchmark::Chronometer meter) {
        std::vector<void*> ptrs;
        ptrs.reserve(1000);

        meter.measure([&] {
            // This block is what gets timed
            for (int i = 0; i < 1000; ++i) {
                ptrs.push_back(pool.allocate(64));
            }
        });

        // Cleanup outside measurement
        for (auto ptr : ptrs) {
            pool.deallocate(ptr);
        }
    };
}

TEST_CASE("Pool Allocator - Cache Performance(Random vs Sequential)",
          "[pool_allocator][benchmark][cachePerformance]") {

    const size_t NUM_OBJECTS = 10000;
    allocator::pool_allocator pool(64, NUM_OBJECTS);

    std::vector<void*> ptrs;
    for (size_t i = 0; i < NUM_OBJECTS; ++i) {
        ptrs.push_back(pool.allocate(64));
    }

    // Sequential access
    BENCHMARK_ADVANCED("Sequential-Access")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            for (auto ptr : ptrs) {
                volatile int* data = static_cast<int*>(ptr);
                *data = 42;
            }
        });
    };

    // Random access
    BENCHMARK_ADVANCED("Random-Access")(Catch::Benchmark::Chronometer meter) {
        std::shuffle(ptrs.begin(), ptrs.end(), std::mt19937{std::random_device{}()});

        meter.measure([&] {
            for (auto ptr : ptrs) {
                volatile int* data = static_cast<int*>(ptr);
                *data = 42;
            }
        });
    };
}

TEST_CASE("Pool allocator - Pool Growth Cost", "[pool_allocator][benchmark][growthCost]") {

    BENCHMARK_ADVANCED("Growth-Performance")(Catch::Benchmark::Chronometer meter) {
        allocator::pool_allocator pool(64, 100);
        std::vector<void*> all_ptrs;

        meter.measure([&] {
            // Force multiple pool allocations
            for (int pool_num = 0; pool_num < 10; ++pool_num) {
                // Fill current pool
                for (size_t i = 0; i < 100; ++i) {
                    all_ptrs.push_back(pool.allocate(64));
                }
                // Next allocation will trigger pool growth
            }
        });

        // Cleanup
        pool.releaseMemory();
    };
}

TEST_CASE("Pool Allocator - Realistic Game Pattern", "[pool_allocator][benchmark][gamePattern]") {
    struct Bullet {
        float position;
        float velocity;
        int damage;
        // other bullet data
    };

    allocator::pool_allocator bullet_pool(sizeof(Bullet), 1000, alignof(Bullet));
    std::vector<void*> active_bullets;

    BENCHMARK_ADVANCED("Game-Simulation")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
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
        });

        // Cleanup
        for (auto ptr : active_bullets) {
            bullet_pool.deallocate(ptr);
        }
    };
}

TEST_CASE("Pool Allocator - Alignment Overhead", "[pool_allocator][benchmark][alignmentOverhead]") {

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
        }
    }
}