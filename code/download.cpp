// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <chrono>
#include <expected>
#include <optional>
#include <string>
#include <utility>

#include <curl/curl.h>

module walng.download;

namespace walng {
namespace detail {

class curl_easy_handle {
private:
  CURL* handle_ = nullptr;

public:
  curl_easy_handle(curl_easy_handle const&) = delete;
  curl_easy_handle& operator=(curl_easy_handle const&) = delete;

  curl_easy_handle(curl_easy_handle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  curl_easy_handle& operator=(curl_easy_handle&& other) noexcept {
    if (this != &other) {
      this->~curl_easy_handle();
      new (this) curl_easy_handle(std::move(other));
    }
    return *this;
  }

  curl_easy_handle() noexcept : curl_easy_handle(::curl_easy_init()) {}

  explicit curl_easy_handle(CURL* handle) noexcept : handle_(handle) {}

  ~curl_easy_handle() {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  void reset() noexcept {
    ::curl_easy_reset(handle_);
  }

  template <typename T>
  auto set_option(CURLoption opt, T&& value) noexcept -> std::expected<void, CURLcode> {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return this->set_option_impl(opt, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return this->set_option_impl(opt, std::string(value).c_str());
    } else {
      return this->set_option_impl(opt, std::forward<T>(value));
    }
  }

  template <typename T>
  auto get_info(CURLINFO info) const -> std::expected<T, CURLcode> {
    if constexpr (std::is_same_v<T, std::string>) {
      return this->get_info_impl<char const*>(info).transform([](char const* value) {
        return std::string(value);
      });
    } else if constexpr (std::is_same_v<T, std::string_view>) {
      return this->get_info_impl<char const*>(info).transform([](char const* value) {
        return std::string_view(value);
      });
    } else {
      return this->get_info_impl<T>(info);
    }
  }

  auto perform() noexcept -> std::expected<void, CURLcode> {
    if (auto const rc = ::curl_easy_perform(handle_); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(rc);
    }
    return {};
  }

private:
  template <typename T>
  auto set_option_impl(CURLoption opt, T&& value) noexcept -> std::expected<void, CURLcode> {
    if (auto const rc = ::curl_easy_setopt(handle_, opt, std::forward<T>(value)); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(rc);
    }
    return {};
  }

  template <typename T>
  auto get_info_impl(CURLINFO info) const -> std::expected<T, CURLcode> {
    T result;
    if (auto const rc = ::curl_easy_getinfo(handle_, info, &result); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(rc);
    }
    return {result};
  }
};

class curl_url_handle {
private:
  CURLU* handle_ = nullptr;

public:
  curl_url_handle(curl_url_handle const& other) : curl_url_handle(::curl_url_dup(other.handle_)) {}

  curl_url_handle& operator=(curl_url_handle const& other) {
    if (this != &other) {
      if (handle_) {
        ::curl_url_cleanup(handle_);
      }
      handle_ = ::curl_url_dup(other.handle_);
    }
    return *this;
  }

  curl_url_handle(curl_url_handle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  curl_url_handle& operator=(curl_url_handle&& other) noexcept {
    if (this != &other) {
      this->~curl_url_handle();
      new (this) curl_url_handle(std::move(other));
    }
    return *this;
  }

  curl_url_handle() noexcept : curl_url_handle(::curl_url()) {}

  explicit curl_url_handle(CURLU* handle) noexcept : handle_(handle) {}

  ~curl_url_handle() {
    if (handle_) {
      ::curl_url_cleanup(handle_);
    }
  }

  explicit curl_url_handle(char const* url) noexcept : curl_url_handle() {
    this->operator=(url);
  }

  curl_url_handle& operator=(char const* url) noexcept {
    ::curl_url_set(handle_, CURLUPART_URL, url, 0);
    return *this;
  }

  explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  auto get() const -> std::optional<std::string> {
    return this->get_url_as_string(CURLUPART_URL);
  }

  auto host() const -> std::optional<std::string> {
    return this->get_url_as_string(CURLUPART_HOST);
  }

  auto path() const -> std::optional<std::string> {
    return this->get_url_as_string(CURLUPART_PATH);
  }

private:
  auto get_url_as_string(CURLUPart part) const -> std::optional<std::string> {
    char* ptr = nullptr;
    ::curl_url_get(handle_, part, &ptr, 0);
    if (ptr == nullptr) {
      return std::nullopt;
    }
    return std::string(ptr);
  }
};

} // namespace detail

namespace {

static auto curl_write_fn(char const* data, size_t size, size_t nmemb, void* userdata) -> size_t {
  auto const response = static_cast<download_response*>(userdata);
  auto const chunk_size = size * nmemb;
  if (!response->content) {
    response->content.emplace();
  }
  response->content->append(data, chunk_size);
  return chunk_size;
};

} // namespace

auto download(char const* url, std::optional<std::chrono::milliseconds> timeout)
    -> std::expected<download_response, std::string> {

  detail::curl_easy_handle handle;
  if (!handle) {
    return std::unexpected("can't init curl");
  }

  download_response response;

  if (auto const result = handle.set_option(CURLOPT_WRITEDATA, &response); !result) {
    return std::unexpected("can't init curl (write data)");
  }
  if (auto const result = handle.set_option(CURLOPT_WRITEFUNCTION, curl_write_fn); !result) {
    return std::unexpected("can't init curl (write function)");
  }
  if (auto const result = handle.set_option(CURLOPT_FOLLOWLOCATION, 1L); !result) {
    return std::unexpected("can't init curl (follow location)");
  }
  if (auto const result = handle.set_option(CURLOPT_URL, url); !result) {
    return std::unexpected("can't init curl (url)");
  }

  if (timeout) {
    if (auto const result = handle.set_option(CURLOPT_TIMEOUT_MS, static_cast<long>(timeout->count())); !result) {
      return std::unexpected("can't init curl (timeout)");
    }
  }

  char error_buffer[CURL_ERROR_SIZE] = {0};
  if (auto const result = handle.set_option(CURLOPT_ERRORBUFFER, error_buffer); !result) {
    return std::unexpected("can't init curl (error buffer)");
  }

  if (auto const result = handle.perform(); !result) {
    return std::unexpected(std::string(error_buffer));
  }
  if (auto const result = handle.get_info<long>(CURLINFO_RESPONSE_CODE); result) {
    response.response_code = static_cast<unsigned>(result.value());
  }
  if (auto result = handle.get_info<std::string>(CURLINFO_EFFECTIVE_URL); result) {
    response.effective_url = std::move(result.value());
  }
  if (auto result = handle.get_info<std::string>(CURLINFO_CONTENT_TYPE); result) {
    response.content_type = std::move(result.value());
  }

  return {std::move(response)};
}

} // namespace walng
