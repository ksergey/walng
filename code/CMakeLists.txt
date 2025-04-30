add_executable(${TargetName})

target_compile_features(${TargetName}
  PRIVATE cxx_std_23)

set_target_properties(${TargetName}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

target_compile_options(${TargetName}
  PRIVATE
    -Wall -Wextra -Wattributes -Wpedantic -Wstrict-aliasing -Wcast-align -g
    -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=
)

target_compile_definitions(${TargetName}
  PRIVATE -DWALNG_VERSION="${VersionString}"
)

target_link_libraries(${TargetName}
  PRIVATE cxxopts::cxxopts yaml-cpp::yaml-cpp pantor::inja)

file(GLOB_RECURSE Sources "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE Headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(${TargetName} PUBLIC ${Headers} ${Sources})

file(COPY config.yaml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

include(GNUInstallDirs)

install(TARGETS ${TargetName} DESTINATION ${CMAKE_INSTALL_BINDIR})
