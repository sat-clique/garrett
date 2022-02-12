cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

project(garrett VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

enable_testing()

find_package(LibArchive REQUIRED)

add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(test)

