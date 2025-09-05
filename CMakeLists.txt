cmake_minimum_required(VERSION 3.23)

project(Project)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror) 
endif()

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Optional: Collect headers for IDEs
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp include/*.h)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include path
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link math library only for non-Windows systems
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
