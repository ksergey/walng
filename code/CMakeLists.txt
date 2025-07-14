set(TargetName walng)

file(GLOB_RECURSE TargetModules "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB_RECURSE TargetSources "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TargetName})
target_compile_features(${TargetName} PRIVATE cxx_std_23)
target_compile_options(${TargetName}
  PRIVATE
    -Wall -Wextra -Wnrvo -Wattributes -Wpedantic -Wstrict-aliasing -Wcast-align -g
)
target_compile_definitions(${TargetName}
  PRIVATE
    -DWALNG_VERSION="${CMAKE_PROJECT_VERSION}"
)
set_target_properties(${TargetName}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(${TargetName}
  PRIVATE
    CURL::libcurl_static cxxopts::cxxopts yaml-cpp::yaml-cpp 3rdparty::inja
)

CMakeUtilsAddTestsFromSourceList(TargetSources
  PREFIX ${TargetName}
  COMPILE_FEATURES cxx_std_23
  COMPILE_OPTIONS -Wall -Wextra -g
  LINK_LIBS doctest::doctest_with_main CURL::libcurl_static)
CMakeUtilsExcludeTestsFromSourceList(TargetSources)

target_sources(${TargetName}
  PRIVATE
    ${TargetSources}
  PRIVATE
    FILE_SET CXX_MODULES FILES
    ${TargetModules}
)

file(COPY config.yaml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


include(GNUInstallDirs)
install(TARGETS ${TargetName} DESTINATION ${CMAKE_INSTALL_BINDIR})
