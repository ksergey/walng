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

std::expected<std::filesystem::path, std::system_error> extractFileNameFromUrl(std::string const& url) noexcept {
  return ::walng::curl::UrlHandle::create()
      .and_then([&](auto&& handle) {
        return handle.setPart(CURLUPART_URL, url).and_then([&] {
          return handle.getPart(CURLUPART_PATH);
        });
      })
      .transform([](::walng::curl::UrlPart part) {
        return std::filesystem::path(part.c_str()).filename();
      });
}

std::expected<void, std::system_error> writeFile(std::filesystem::path const& path, std::string_view content) noexcept {
  FILE* file = ::fopen(path.c_str(), "w");
  if (!file) {
    return std::unexpected(std::system_error(errno, std::system_category(), "failed to open file for writting"));
  }

  // TODO: check result
  ::fwrite(content.data(), content.size(), 1, file);
  ::fclose(file);

  return {};
}

std::expected<std::filesystem::path, std::system_error> downloadFileOrGetFromCache(std::string const& url) noexcept {
  using namespace walng;

  auto filename = extractFileNameFromUrl(url);
  if (!filename) {
    return std::unexpected(filename.error());
  }

  auto request = Request();
  if (auto const rc = request.prepare(url); !rc) {
    return std::unexpected(rc.error());
  }

  auto response = request.perform();
  if (!response) {
    return std::unexpected(response.error());
  }

  auto cachePath = getCachePath();
  if (!cachePath.has_value()) {
    return std::unexpected(cachePath.error());
  }
  auto const themesPath = cachePath.value() / "themes";
  std::error_code ec;
  create_directories(themesPath, ec);
  if (ec) {
    return std::unexpected(std::system_error(ec, "can't create themes cache dir"));
  }

  auto const themeFilePath = themesPath / *filename;
  if (auto const rc = writeFile(themeFilePath, response->body()); !rc) {
    return std::unexpected(response.error());
  }

  return themeFilePath;
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
      auto configPath = walng::getConfigPath();
      if (!configPath.has_value()) {
        throw configPath.error();
      }
      return configPath.value() / "config.yaml";
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
        throw downloadResult.error();
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
