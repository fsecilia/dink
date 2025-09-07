/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <cstddef>
#include <utility>

// clang-format off

// define set of platform implementations
#define dink_page_size_impl_posix 1

// determine which platform implementation to use
#if !defined dink_page_size_impl
    #if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
        #define dink_page_size_impl dink_page_size_impl_posix
    #endif
#endif

#if !defined dink_page_size_impl
    #error Could not determine platform page size implementation.
#endif

// include platform-specific headers
#if dink_page_size_impl == dink_page_size_impl_posix
    #include <unistd.h>
#endif

// clang-format on

namespace dink {

//! fallback page size used if os query fails
constexpr auto const fallback_page_size = 4096; // 4k pages

//! callable that returns size_t
template <typename page_size_t>
concept page_size = requires(page_size_t page_size) {
    { page_size() } -> std::same_as<std::size_t>;
};

#if dink_page_size_impl == dink_page_size_impl_posix

namespace posix {

//! gets page size directly from sysconf
template <typename api_t>
class page_size_t
{
public:
    static constexpr auto const sysconf_page_size_name = _SC_PAGESIZE;

    auto operator()() const noexcept -> std::size_t
    {
        auto const result = api_.sysconf(sysconf_page_size_name);
        if (result > 0) return static_cast<size_t>(result);
        return fallback_page_size;
    }

    page_size_t(api_t api = {}) noexcept : api_{std::move(api)} {}

private:
    [[no_unique_address]] api_t api_;
};

struct page_size_api_t
{
    auto sysconf(int name) const noexcept -> long int { return ::sysconf(name); }
};

} // namespace posix

using page_size_t = posix::page_size_t<posix::page_size_api_t>;

#endif

} // namespace dink
