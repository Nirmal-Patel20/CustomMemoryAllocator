//add this cpp file to just not have an empty directory(which CMake doesn't like)
//this file is not compiled or used in any way
//i will add some examples later when i have built the library a bit more
#include <iostream>
#include <allocators/pool_allocator.hpp>

int main() {
    greet();
    return 0;
}