cmake_minimum_required(VERSION 3.31.6 FATAL_ERROR)

project(walng LANGUAGES CXX VERSION 0.0.8)

include(cmake/CMakeUtils.cmake)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_subdirectory(deps EXCLUDE_FROM_ALL)

enable_testing()

add_subdirectory(code)
add_subdirectory(man)
