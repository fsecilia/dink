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
#define groundwork_memory_impl_fallback 0
#define groundwork_memory_impl_posix 1

// determine which platform implementation to use
#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    #define groundwork_memory_impl groundwork_memory_impl_posix
#else
    #define groundwork_memory_impl groundwork_memory_impl_fallback
#endif

// include platform-specific headers
#if groundwork_memory_impl == groundwork_memory_impl_posix
    #include <unistd.h>
#endif

// clang-format on

namespace dink {
namespace memory {

namespace fallback {

//! fallback memory_t implementation using stdlib where possible or reasonable constants otherwise
class impl_t
{
public:
    /*!
        estimates cache line size using stdlib

        This implementation uses std::hardware_destructive_interference_size verbatim. This is the best estimation at
        compile time. It is fixed at compile time, though, so it will underestimate when running the same compiled
        binary on more recent hardware with a larger cache.
    */
    auto cache_line_size() const noexcept -> std::size_t { return std::hardware_destructive_interference_size; }

    /*!
        reasonable estimate of current os page sizes

        Page size does not have a stdlib constant, so we default to 4k.
    */
    auto page_size() const noexcept -> std::size_t { return 4096; }
};

} // namespace fallback

#if groundwork_memory_impl == groundwork_memory_impl_fallback
using memory_t = memory::fallback_t;
#endif

#if groundwork_memory_impl == groundwork_memory_impl_posix
namespace posix {

//! posix memory_t implementation using sysconf
template <typename api_t, typename fallback_t>
class impl_t
{
public:
    static constexpr auto const sysconf_cache_line_size_name = _SC_LEVEL1_DCACHE_LINESIZE;
    static constexpr auto const sysconf_page_size_name = _SC_PAGESIZE;

    //! gets level 1 data cache line size directly from sysconf
    auto cache_line_size() const noexcept -> std::size_t
    {
        auto const result = api_.sysconf(sysconf_cache_line_size_name);
        if (result > 0) return static_cast<size_t>(result);
        return fallback_.cache_line_size();
    }

    //! gets page size directly from sysconf
    auto page_size() const noexcept -> std::size_t
    {
        auto const result = api_.sysconf(sysconf_page_size_name);
        if (result > 0) return static_cast<size_t>(result);
        return fallback_.page_size();
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

} // namespace memory

//! platform-specific memory metrics
using memory_t = memory::impl_t;

} // namespace dink
