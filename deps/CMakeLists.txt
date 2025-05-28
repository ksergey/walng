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

set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_CURL_EXE OFF)
set(CURL_DISABLE_INSTALL ON)
set(ENABLE_ARES OFF)
set(ENABLE_THREADED_RESOLVER OFF)
set(ENABLE_DEBUG OFF)
set(CURL_ENABLE_EXPORT_TARGET OFF)
set(HTTP_ONLY ON)
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_MISC_DOCS OFF)
set(ENABLE_CURL_MANUAL OFF)
set(USE_LIBIDN2 OFF)
set(CURL_USE_LIBPSL OFF)
set(CURL_USE_LIBSSH2 OFF)
set(ENABLE_UNIX_SOCKETS OFF)
set(BUILD_TESTING OFF)
set(BUILD_EXAMPLES OFF)
FetchContent_Declare(curl
  URL https://github.com/curl/curl/releases/download/curl-8_14_0/curl-8.14.0.tar.bz2
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(curl)
