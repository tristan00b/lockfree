#
# Project Root Directory
#

cmake_minimum_required(VERSION 3.17)

project(Lockfree LANGUAGES CXX VERSION 0.0.1)

# CMake config
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

## Don't override user provided prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}"
    CACHE PATH "Path prefix for relative install paths"
    FORCE
    )
endif()

## Test config
enable_testing()
include(catch2.dependency)

add_subdirectory(src)
add_subdirectory(tests)


