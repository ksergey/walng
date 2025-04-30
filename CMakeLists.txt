cmake_minimum_required(VERSION 3.24)

set(VersionString "0.0.1")

project(walng CXX)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_subdirectory(deps EXCLUDE_FROM_ALL)

set(TargetName walng)

add_subdirectory(code)
add_subdirectory(man)
