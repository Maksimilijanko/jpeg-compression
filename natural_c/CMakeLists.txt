cmake_minimum_required(VERSION 3.15)

project(JPEG_Compression_Natural_C LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Look for headers in current source dir -> include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add debugging flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Gather all source files, use pattern matching and recursion (GLOB_RECURSE)
# Place to variable SOURCES
# GLOB_RECURSE - match regular expression and traverse subdirectories
# SOURCES - variable holding all found files (file paths)
# CONFIGURE_DEPENDS - detect changes before compiling
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.c")

# Create natural_c target
# A target is a single compilation toolchain run - from compiling to the linking stage and generating a single artifact (an executable or a lib)
# First argument is the name of the executable
# Second argument is the list of source files
add_executable(jpeg_enc_nat_c ${SOURCES})

target_compile_options(jpeg_enc_nat_c PRIVATE -fsanitize=address -g)
target_link_options(jpeg_enc_nat_c PRIVATE -fsanitize=address)

# Link math library
target_link_libraries(jpeg_enc_nat_c m)

