cmake_minimum_required(VERSION 3.16)
project(redis_server CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Root source directory
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# Automatically collect all .cpp files in src and its subdirectories
file(GLOB_RECURSE SOURCES
    "${SRC_DIR}/*.cpp"
)
find_package(OpenSSL REQUIRED)

# Add include directory
include_directories("${SRC_DIR}/include")
include_directories(${OpenSSL_INCLUDE_DIR})
# Create executable
add_executable(redis_server ${SOURCES})

target_link_libraries(redis_server PRIVATE OpenSSL::Crypto OpenSSL::SSL)
# Optional: more strict warnings for development
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(redis_server PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable) 
endif()
