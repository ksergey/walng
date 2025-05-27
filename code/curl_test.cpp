// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "curl_test.h"

#include <concepts>
#include <expected>
#include <functional>
#include <print>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include <curl/curl.h>

namespace curl {
namespace detail {

template <auto>
struct CallbackTraits;

template <typename ClassT, typename ResultT, ResultT ClassT::* ptr>
struct CallbackTraits<ptr> {
  using Class = ClassT;
};

} // namespace detail

struct CurlErrorCategory final : public std::error_category {
  [[nodiscard]] char const* name() const noexcept override {
    return "CurlError";
  }
  [[nodiscard]] std::string message(int errorCode) const noexcept {
    return ::curl_easy_strerror(static_cast<CURLcode>(errorCode));
  }
};

[[nodiscard]] std::error_category const& getCurlErrorCategory() noexcept {
  static CurlErrorCategory errorCategory;
  return errorCategory;
}

template <typename T = void, class E = std::error_code>
using Result = std::expected<T, E>;

constexpr Result<> success() noexcept {
  return {};
}

[[nodiscard]] decltype(auto) makeCurlErrorCode(CURLcode code) noexcept {
  return std::unexpected(std::error_code(static_cast<int>(code), getCurlErrorCategory()));
}

template <class T>
concept MaybeString = std::is_convertible_v<T, std::string_view>;

template <typename T, typename Fn>
concept BodyCallback = requires(T&& obj, Fn&& fn, std::span<char const> data) {
  { std::invoke(std::forward<Fn>(fn), std::forward<T>(obj), data) };
};

class Request {
private:
  CURL* handle_ = nullptr;

public:
  Request(Request const&) = delete;
  Request& operator=(Request const&) = delete;

  Request(Request&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  Request& operator=(Request&& other) noexcept {
    if (this != &other) {
      this->~Request();
      new (this) Request(std::move(other));
    }
    return *this;
  }

  Request(std::nullptr_t) {}

  Request() {
    handle_ = ::curl_easy_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl easy handle");
    }
  }

  ~Request() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  template <typename T>
    requires MaybeString<T>
  Result<> setURL(T value) {
    return this->setOption(CURLOPT_URL, value);
  }

  template <typename T>
    requires MaybeString<T>
  Result<> setUserAgent(T value) {
    return this->setOption(CURLOPT_USERAGENT, value);
  }

  Result<> setFollowLocation(bool value) {
    return this->setOption(CURLOPT_FOLLOWLOCATION, value);
  }

  template <auto fn>
    requires std::invocable<decltype(fn), std::span<char const>>
  Result<> setCallback() noexcept {
    return this->setOption(CURLOPT_WRITEFUNCTION, curlWriteCallbackFn<fn>);
  }

  template <auto fn>
    requires std::invocable<decltype(fn), typename detail::CallbackTraits<fn>::Class, std::span<char const>>
  Result<> setCallback(typename detail::CallbackTraits<fn>::Class& obj) noexcept {
    if (auto const rc = this->setOption(CURLOPT_WRITEFUNCTION, curlWriteCallbackMemberFn<fn>); !rc) {
      return rc;
    }
    return this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&obj));
  }

  [[nodiscard]] Result<> perform() noexcept {
    if (auto const ec = ::curl_easy_perform(handle_); ec != CURLE_OK) {
      return makeCurlErrorCode(ec);
    }
    return success();
  }

private:
  template <typename T>
  Result<> setOption(::CURLoption option, T value) noexcept {
    if (auto const ec = ::curl_easy_setopt(handle_, option, value); ec != CURLE_OK) {
      return makeCurlErrorCode(ec);
    }
    return success();
  }

  Result<> setOption(::CURLoption option, std::string const& value) noexcept {
    return this->setOption(option, value.c_str());
  }

  Result<> setOption(::CURLoption option, std::string_view value) {
    return this->setOption(option, std::string(value).c_str());
  }

  Result<> setOption(::CURLoption option, bool value) noexcept {
    return this->setOption(option, static_cast<long>(value));
  }

  template <auto fn>
  static std::size_t curlWriteCallbackFn(
      char* ptr, std::size_t size, std::size_t nmemb, [[maybe_unused]] void* userdata) {
    auto const data = std::span<char const>(ptr, size * nmemb);
    std::invoke(fn, data);
    return data.size();
  }

  template <auto fn>
  static std::size_t curlWriteCallbackMemberFn(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    using Obj = typename detail::CallbackTraits<fn>::Class;
    auto const data = std::span<char const>(ptr, size * nmemb);
    auto const obj = static_cast<Obj*>(userdata);
    std::invoke(fn, *obj, data);
    return data.size();
  }
};

} // namespace curl

void onData(std::span<char const> data) noexcept {
  std::print("got data chunk (size = {})\n", data.size());
}

struct Downloader {
  void process(std::span<char const> data) noexcept {
    std::print("Downloader: got data chunk (size = {})\n", data.size());
  }
};

void curl_test() {
  try {
    auto request = curl::Request();
    // request.setURL("https://api.github.com/repos/tinted-theming/schemes/contents/base16");
    request.setURL("https://api.github.com/repos/tinted-theming/schemes/contents/scripts");
    request.setUserAgent("BobrKurwa 2x0.5");
    request.setFollowLocation(true);
    // request.setCallback<&onData>();

    Downloader downloader;
    request.setCallback<&Downloader::process>(downloader);

    if (auto const rc = request.perform(); !rc) {
      throw std::system_error(rc.error(), "perform");
    }

  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
  }
}
