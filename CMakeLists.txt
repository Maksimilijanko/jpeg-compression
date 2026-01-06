cmake_minimum_required(VERSION 3.10)

project(JPEG_Encoder_Suite)

# All executable artifacts go to /build/bin
# This will initialize every target's RUNTIME_OUTPUT_DIRECTORY property
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Include natural C implementation
# This will cause CMake to create natural_c subdirectory inside build directory as well.
add_subdirectory(natural_c)

# Include TI-specific build
# add_subdirectory(ti)






# CMake predifined variables:
#   - CMAKE_BINARY_DIR is the build directory and CMAKE_SOURCE_DIR is the source directory
#   - CMAKE_BINARY_DIR is defined by CMake automatically, based on where the command is run
#   - CMAKE_SOURCE_DIR is defined by CMake automatically, based on where the top-level CMakeLists.txt is located