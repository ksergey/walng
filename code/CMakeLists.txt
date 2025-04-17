set(TargetName walng)

add_executable(${TargetName})
target_compile_features(${TargetName}
  PRIVATE cxx_std_23)
set_target_properties(${TargetName}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_compile_options(${TargetName}
  PRIVATE -Wall -Wextra -Wattributes -Wpedantic -Wstrict-aliasing -Wcast-align -g)
target_link_libraries(${TargetName}
  PRIVATE cxxopts::cxxopts yaml-cpp::yaml-cpp pantor::inja)

file(GLOB_RECURSE Sources "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE Headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(${TargetName} PUBLIC ${Headers} ${Sources})

file(COPY gruvbox-dark.yaml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY rofi-colors.rasi DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
