#include "allocator/pool_allocator.hpp"
#include <iostream>
#include <thread>
#include <vector>

void cleanscreen() {
    // Clear the console screen
    std::cout
        << "\033[2J\033[1;1H"; // ANSI escape code to clear the screen and move cursor to top-left
}

void pool_allocator_example(); // forward declaration

int main() {

    pool_allocator_example(); // pool allocator is mostly fixed size allocations, it mostly shines
                              // in game dev scenarios
    //                        // but i tried to make a simple example here

    return 0;
}

void pool_allocator_example() {
    cleanscreen();
    std::cout << ">>>>>> Basic usage of Pool allocator <<<<<<\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause for 2 seconds to read the message

    // create a pool allocator for int type
    allocator::Pool_Allocator pool(sizeof(int), 100, alignof(int));

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

    std::cout << "Allocated integers: " << *allocated_ptrs[0] << ", " << *allocated_ptrs[1] << ", "
              << *allocated_ptrs[2] << "\n";
    std::cout << "Allocated size: " << pool.getAllocatedSize() << " bytes\n";
    std::cout << "Object size: " << pool.getObjectSize() << " bytes\n";

    pool.deallocate(allocated_ptrs[1]);
    std::cout << "Deallocated p2\n";
    std::cout << "Allocated size after deallocation: " << pool.getAllocatedSize() << " bytes\n";
    std::this_thread::sleep_for(std::chrono::seconds(4)); // Pause for 4 seconds to read the message

    // reset the pool
    std::cout << "Pool reset\n";
    pool.reset();

    int* ptr = static_cast<int*>(pool.allocate(sizeof(int)));
    *ptr = 42;

    // release all memory
    // after this the allocator cannot be used again unless we call reset()
    pool.releaseMemory();

    std::cout << "Released all memory from pool\n";
    std::cout << "Trying to allocate after releaseMemory() should throw error\n";
    try {
        pool.allocate(sizeof(int));
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }

    std::cout << ">>>>>> Example complete. Exiting <<<<<<\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause for 2 seconds before exit
}