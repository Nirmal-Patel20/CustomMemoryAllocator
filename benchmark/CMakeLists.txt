# for windows, package managers like vcpkg or conan may install catch2 in non-standard locations
if(WIN32)
    # Add vcpkg default install location
    list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/packages/catch2_x64-mingw-static/share")

    # Add Program Files catch2 installation location
    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/Catch2")    

endif()

# Try to find Catch2 via find_package first
find_package(Catch2 3 CONFIG QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "=========catch2=========")
    message(STATUS ">>> Catch2 not found, fetching via FetchContent <<<")
    include(FetchContent)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(catch2)
endif()

# Print Catch2 version(escape codes for bold yellow text)
message(STATUS ">>> Catch2 version: ${Catch2_VERSION} <<<")

add_executable(benchmarks 
        Pool_allocator_benchmark.cpp
        stack_allocator_benchmark.cpp
)

target_link_libraries(benchmarks 
    PRIVATE 
        allocator 
        Catch2::Catch2WithMain
)
target_include_directories(benchmarks PRIVATE ../include)

include(CTest)
include(Catch)
catch_discover_tests(benchmarks)