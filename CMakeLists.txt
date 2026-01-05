cmake_minimum_required(VERSION 3.15)

project(JPEG_Compression LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Add debugging flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Gather all source files, use pattern matching and recursion (GLOB_RECURSE)
# Place to variable SOURCES
file(GLOB_RECURSE SOURCES "src/*.c")

# Create the executable
# First argument is the name of the executable
# Second argument is the list of source files
add_executable(jpeg_encoder ${SOURCES})

# Configure output directory
set_target_properties(jpeg_encoder PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# CMAKE_BINARY_DIR is the build directory and CMAKE_SOURCE_DIR is the source directory
# CMAKE_BINARY_DIR is defined by CMake automatically, based on where the command is run
# CMAKE_SOURCE_DIR is defined by CMake automatically, based on where the top-level CMakeLists.txt is located