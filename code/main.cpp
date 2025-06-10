// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <expected>
#include <filesystem>
#include <print>

#include <cxxopts.hpp>

#include "Request.h"
#include "generate.h"
#include "utils.h"
#include "version.h"

std::expected<std::filesystem::path, std::error_code> downloadFileOrGetFromCache(std::string const& url) noexcept {
  using namespace walng;

  return Request::create()
      .and_then([&](auto&& request) -> std::expected<Response, std::error_code> {
        if (auto const rc = request.setURL(url.c_str()); !rc) {
          return std::unexpected(rc.error());
        }
        if (auto const rc = request.setTimeout(std::chrono::seconds(5)); !rc) {
          return std::unexpected(rc.error());
        }
        return request.perform();
      })
      .and_then([&](auto&& response) -> std::expected<std::filesystem::path, std::error_code> {
        auto path = cacheBasePath() / "theme";
        std::error_code ec;
        create_directories(path, ec);
        if (!ec) {
          return std::unexpected(ec);
        }
        path /= "theme.yaml";
        FILE* file = ::fopen(path.c_str(), "w");
        if (!file) {
          return std::unexpected(std::error_code(errno, std::system_category()));
        }
        ::fwrite(response.body().data(), response.body().size(), 1, file);
        ::fclose(file);
        return path;
      });
}

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("walng", "color template generator for base16 framework\n");

    // clang-format off
    options.add_options()
      ("config", "path to config file", cxxopts::value<std::string>(), "PATH")
      ("theme-file", "path to theme file", cxxopts::value<std::string>(), "PATH")
      ("theme-url", "url to theme file", cxxopts::value<std::string>(), "URL")
      ("help", "prints the help and exit")
      ("version", "prints the version and exit")
    ;
    // clang-format on

    auto const result = options.parse(argc, argv);

    if (result.count("help")) {
      std::print(stdout, "{}\n", options.help());
      return EXIT_FAILURE;
    }

    if (result.count("version")) {
      std::print(stdout, "walng {}\n", walng::version);
      return EXIT_FAILURE;
    }

    std::filesystem::path const configPath = [&] {
      if (result.count("config")) {
        return std::filesystem::path(result["config"].as<std::string>());
      }
      return walng::configPathBase() / "config.yaml";
    }();

    if (result.count("theme-file") > 0) {
      if (result.count("theme-url") > 0) {
        throw std::runtime_error("argument `--theme-file` and `--theme-url` can't be set simultaneously");
      }
      std::filesystem::path const themePath = result["theme-file"].as<std::string>();
      walng::generate(configPath, themePath);
    } else if (result.count("theme-url") > 0) {
      std::string const& themeUrl = result["theme-url"].as<std::string>();
      auto const downloadResult = downloadFileOrGetFromCache(themeUrl);
      if (!downloadResult) {
        throw std::system_error(downloadResult.error(), "failed to download theme");
      }
      walng::generate(configPath, downloadResult.value());
    } else {
      throw std::runtime_error("argument `--theme-file` or `--theme-url` should be set");
    }

  } catch (std::exception const& e) {
    std::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
