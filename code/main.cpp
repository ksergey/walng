// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <expected>
#include <filesystem>
#include <print>

#include <cxxopts.hpp>

#include "Request.h"
#include "generate.h"
#include "net/download.h"
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

std::expected<void, std::error_code> writeFile(std::filesystem::path const& path, std::string_view content) noexcept {
  FILE* file = ::fopen(path.c_str(), "w");
  if (!file) {
    return std::unexpected(std::error_code(errno, std::system_category()));
  }

  // TODO: check result
  ::fwrite(content.data(), content.size(), 1, file);
  ::fclose(file);

  return {};
}

/// Download file. Return error or path where content stored
std::expected<std::filesystem::path, std::system_error> downloadFile(std::string const& url) noexcept {
  auto response = walng::net::download(url);
  if (!response) {
    return std::unexpected(response.error());
  }

  if (response->code != 200) {
    return std::unexpected(std::error_code(ENOENT, std::system_category()));
  }

  auto cachePath = walng::getCachePath();
  if (!cachePath.has_value()) {
    return std::unexpected(cachePath.error());
  }

  auto const themesPath = cachePath.value() / "themes";
  std::error_code ec;
  create_directories(themesPath, ec);
  if (ec) {
    return std::unexpected(ec);
  }

  auto themeFilePath = themesPath / response->filename;
  if (auto const rc = writeFile(themeFilePath, response->content); !rc) {
    return std::unexpected(response.error());
  }

  return {std::move(themeFilePath)};
}

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("walng", "color template generator for base16 framework\n");

    // clang-format off
    options.add_options()
      ("config", "path to config file", cxxopts::value<std::string>(), "PATH")
      ("theme", "path or url to theme file", cxxopts::value<std::string>(), "PATH or URL")
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
        throw std::system_error(configPath.error());
      }
      return configPath.value() / "config.yaml";
    }();

    if (result.count("theme") == 0) {
      throw std::runtime_error("argument `--theme` should be set");
    }
    auto const& theme = result["theme"].as<std::string>();

    if (theme.starts_with("http://") || theme.starts_with("https://")) {
      if (auto rc = downloadFile(theme); rc) {
        walng::generate(configPath, *rc);
      } else {
        throw std::system_error(rc.error());
      }
    } else {
      walng::generate(configPath, theme);
    }

  } catch (std::exception const& e) {
    std::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
