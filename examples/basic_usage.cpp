#include "allocator/pool_allocator.hpp"
#include <iostream>
#include <thread>
#include <vector>

void pool_allocator_example(); // forward declaration

int main() {

    pool_allocator_example(); // pool allocator is mostly fixed size allocations, it mostly shines
                              // in game dev scenarios
    //                        // but i tried to make a simple example here

    return 0;
}

void pool_allocator_example() {
    std::cout << ">>>>>> Basic usage of Pool allocator <<<<<<\n";

    // create a pool allocator for int type
    allocator::pool_allocator pool(sizeof(int), 100, alignof(int));

    // store allocated pointers for later deallocation
    std::vector<int*> allocated_ptrs;

    // allocate 10 integers
    for (int i = 0; i < 10; ++i) {
        int* ptr = static_cast<int*>(pool.allocate(sizeof(int)));
        allocated_ptrs.push_back(ptr);
    }

    *allocated_ptrs[0] = 10;
    *allocated_ptrs[1] = 20;
    *allocated_ptrs[2] = 30;

    std::cout << "Allocated size: " << pool.getAllocatedSize()
              << " bytes\n"; // this funct dont give pool size but allocated size in pool
    std::cout << "Object size: " << pool.getObjectSize() << " bytes\n";

    pool.deallocate(allocated_ptrs[1]); // deallocate the second integer
    allocated_ptrs[1] = nullptr;        // mark as nullptr to avoid dangling pointer
    std::cout << "Allocated size after deallocation: " << pool.getAllocatedSize() << " bytes\n";

    // reset the pool - this will free all allocations
    pool.reset();

    std::cout << "Allocated size after reset: " << pool.getAllocatedSize()
              << " bytes\n"; // should be 0

    // release all memory
    // after this the allocator cannot be used again unless we call reset()
    pool.releaseMemory();

    std::cout << "Released all memory from pool\n";
    std::cout << "Trying to allocate after releaseMemory() should throw error\n";

    try {
        int* ptr2 = static_cast<int*>(pool.allocate(sizeof(int)));
        *ptr2 = 100; // This should not be reached
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    } catch (const std::bad_alloc& e) {
        std::cout << "Caught expected: " << e.what() << "\n";
    }

    std::cout << ">>>>>> Example complete. Exiting <<<<<<\n";
}