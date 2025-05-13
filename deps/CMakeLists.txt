include(FetchContent)

FetchContent_Declare(cxxopts
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.2.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(cxxopts)

FetchContent_Declare(yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/2f86d13775d119edbb69af52e5f566fd65c6953b.zip
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(json)

add_library(inja INTERFACE)
target_include_directories(inja INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(inja INTERFACE nlohmann_json::nlohmann_json)
add_library(3rdparty::inja ALIAS inja)
