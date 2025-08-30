/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cstddef>
#include <new>
#include <utility>

// clang-format off

// define set of platform implementations
#define groundwork_cache_line_size_impl_fallback 0
#define groundwork_cache_line_size_impl_posix 1

// determine which platform implementation to use
#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    #define groundwork_cache_line_size_impl groundwork_cache_line_size_impl_posix
#else
    #define groundwork_cache_line_size_impl groundwork_cache_line_size_impl_fallback
#endif

// include platform-specific headers
#if groundwork_cache_line_size_impl == groundwork_cache_line_size_impl_posix
    #include <unistd.h>
#endif

// clang-format on

namespace dink {
namespace cache_line_size {

namespace fallback {

/*!
    fallback implementation of cache_line_size using stdlib

    This implementation uses std::hardware_destructive_interference_size verbatim. It is always available and it is the
    best estimation at compile time. It is fixed at compile time, though, so it will underestimate when running the
    same compiled binary on more recent hardware with a larger cache.
*/
class impl_t
{
public:
    auto operator()() const noexcept -> std::size_t { return std::hardware_destructive_interference_size; }
};

} // namespace fallback

#if groundwork_cache_line_size_impl == groundwork_cache_line_size_impl_fallback
using cache_line_size_t = cache_line_size::fallback_t;
#endif

#if groundwork_cache_line_size_impl == groundwork_cache_line_size_impl_posix
namespace posix {

/*!
    posix implementation of cache_line_size using sysconf

    The posix implementation gets the level 1 data cache line size directly from sysconf.
*/
template <typename api_t, typename fallback_t>
class impl_t
{
public:
    static constexpr auto const sysconf_name = _SC_LEVEL1_DCACHE_LINESIZE;

    auto operator()() const noexcept -> std::size_t
    {
        auto result = api_.sysconf(sysconf_name);
        if (result > 0) return static_cast<size_t>(result);
        return fallback_();
    }

    impl_t(api_t api, fallback_t fallback) noexcept : api_{std::move(api)}, fallback_{std::move(fallback)} {}

private:
    [[no_unique_address]] api_t api_;
    [[no_unique_address]] fallback_t fallback_;
};

struct api_t
{
    auto sysconf(int name) const noexcept -> long int { return ::sysconf(name); }
};

} // namespace posix

using impl_t = posix::impl_t<posix::api_t, fallback::impl_t>;
#endif

} // namespace cache_line_size

using cache_line_size_t = cache_line_size::impl_t;

} // namespace dink
