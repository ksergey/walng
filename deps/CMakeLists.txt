include(FetchContent)

FetchContent_Declare(cxxopts
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.2.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(cxxopts)

FetchContent_Declare(yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(inja
  URL https://github.com/pantor/inja/archive/refs/tags/v3.4.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(inja)
