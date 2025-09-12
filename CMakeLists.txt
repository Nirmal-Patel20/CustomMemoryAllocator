cmake_minimum_required(VERSION 3.16)
project(MemoryAllocator VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/MDd /wd4101) # Suppress unused variable warning in Debug (4101 = unreferenced local variable)
    else()
        add_compile_options(/MD /O2)
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Debug: Keep warnings but suppress unused variable warnings
        add_compile_options(-g -O0 -Wall -Wextra -Wpedantic -Wno-unused-variable)
    else()
        # Release: Full warnings
        add_compile_options(-Wall -Wextra -Wpedantic -O3 -DNDEBUG)
    endif()
endif()

# Build type specific defines
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ALLOCATOR_DEBUG)
    add_compile_definitions(ALLOCATOR_ENABLE_TRACKING)
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Optional: Collect headers for IDEs
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp include/*.h)

# Create the main library
add_library(allocator ${SOURCES})

# Include path
target_include_directories(allocator
    PUBLIC include
    PRIVATE src
 )

option(BUILD_EXAMPLES "Build example programs" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
