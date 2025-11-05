#include "allocator/buddy_allocator.hpp"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

TEST_CASE("Buddy Allocator - Allocation Speed(Pool vs Malloc)(1024 bytes)",
          "[buddy_allocator][comparison][speed]") {
    const size_t NUM_ALLOCATIONS = 10000;
    const size_t OBJECT_SIZE = 1024; // 1KB

    // Test buddy
    BENCHMARK_ADVANCED("Buddy-Variable-Sizes")(Catch::Benchmark::Chronometer meter) {
        allocator::buddy_allocator buddy(128 * 1024 * 1024); // 128MB
        std::vector<void*> ptrs;
        ptrs.reserve(NUM_ALLOCATIONS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
                void* p = buddy.allocate(OBJECT_SIZE);
                if (p) {
                    ptrs.push_back(p);
                }
            }
        });

        for (auto ptr : ptrs) {
            buddy.deallocate(ptr);
        }
    };

    // Test malloc
    BENCHMARK_ADVANCED("Malloc-Variable-Sizes")(Catch::Benchmark::Chronometer meter) {
        std::vector<void*> ptrs;
        ptrs.reserve(NUM_ALLOCATIONS);

        meter.measure([&] {
            for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
                ptrs.push_back(malloc(OBJECT_SIZE));
            }
        });

        for (auto ptr : ptrs) {
            free(ptr);
        }
    };
}

// Benchmark to compare coalescing performance of buddy allocator vs malloc/free
TEST_CASE("Buddy Allocator - Coalescing Performance",
          "[buddy_allocator][Performance][coalescing][comparison]") {

    BENCHMARK_ADVANCED("Buddy-With-Coalescing")(Catch::Benchmark::Chronometer meter) {
        allocator::buddy_allocator buddy(64 * 1024 * 1024);

        meter.measure([&] {
            // Allocate many blocks
            std::vector<void*> ptrs;
            for (int i = 0; i < 1000; ++i) {
                ptrs.push_back(buddy.allocate(1024)); // 1KB each
            }

            // Deallocate in order (maximum coalescing)
            for (auto ptr : ptrs) {
                buddy.deallocate(ptr); // Triggers merging
            }
        });
    };

    BENCHMARK_ADVANCED("Malloc-No-Coalescing")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            std::vector<void*> ptrs;
            for (int i = 0; i < 1000; ++i) {
                ptrs.push_back(malloc(1024));
            }

            for (auto ptr : ptrs) {
                free(ptr); // No coalescing logic
            }
        });
    };
}

// Benchmark to test allocation performance for different size classes
TEST_CASE("Buddy Allocator - Size Impact", "[buddy_allocator][sizes]") {

    BENCHMARK_ADVANCED("Buddy-Small-Sizes")(Catch::Benchmark::Chronometer meter) {
        allocator::buddy_allocator buddy(64 * 1024 * 1024);
        std::vector<void*> ptrs;

        meter.measure([&] {
            for (int i = 0; i < 50; ++i) {
                void* p = buddy.allocate(200); // Rounds to 256B
                if (p) { // ensure allocation succeeded otherwise deallocation will fail slightly
                         // without termination
                    ptrs.push_back(p);
                }
            }
        });

        for (auto ptr : ptrs) {
            buddy.deallocate(ptr);
        }
    };

    BENCHMARK_ADVANCED("Buddy-Medium-Sizes")(Catch::Benchmark::Chronometer meter) {
        allocator::buddy_allocator buddy(64 * 1024 * 1024);
        std::vector<void*> ptrs;

        meter.measure([&] {
            for (int i = 0; i < 25; ++i) {
                void* p = buddy.allocate(5000); // Rounds to 8KB
                if (p) {
                    ptrs.push_back(p);
                }
            }
        });

        for (auto ptr : ptrs) {
            buddy.deallocate(ptr);
        }
    };

    BENCHMARK_ADVANCED("Buddy-Large-Sizes")(Catch::Benchmark::Chronometer meter) {
        allocator::buddy_allocator buddy(64 * 1024 * 1024);
        std::vector<void*> ptrs;

        meter.measure([&] {
            for (int i = 0; i < 10; ++i) {
                void* p = buddy.allocate(100000); // Rounds to 128KB
                if (p) {
                    ptrs.push_back(p);
                }
            }
        });

        for (auto ptr : ptrs) {
            buddy.deallocate(ptr);
        }
    };
}

TEST_CASE("Buddy Allocator - Fragmentation Pattern",
          "[buddy_allocator][benchmark][fragmentation]") {

    BENCHMARK_ADVANCED("Buddy-Random-Order")(Catch::Benchmark::Chronometer meter) {

        meter.measure([&] {
            allocator::buddy_allocator buddy(64 * 1024 * 1024);
            std::vector<void*> ptrs;

            // Allocate various sizes
            for (int i = 0; i < 1000; ++i) {
                ptrs.push_back(buddy.allocate(1024));
            }

            // Deallocate in random order
            std::shuffle(ptrs.begin(), ptrs.end(), std::mt19937{std::random_device{}()});

            for (auto ptr : ptrs) {
                buddy.deallocate(ptr);
            }
        });
    };
}