cmake_minimum_required(VERSION 3.24)

set(VersionString "0.0.5")

project(walng CXX)

include(cmake/CMakeUtils.cmake)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_subdirectory(deps EXCLUDE_FROM_ALL)

set(TargetName walng)

enable_testing()

add_subdirectory(code)
add_subdirectory(man)
