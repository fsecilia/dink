/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/type_map.hpp>
#include <dink/test.hpp>
#include <type_traits>

namespace dink {
namespace {

struct requested_i
{
    virtual auto virtual_method() const noexcept -> void = 0;

protected:
    ~requested_i() noexcept = default;
};

struct resolved_t : public requested_i
{
    auto virtual_method() const noexcept -> void override {}
};

} // namespace

template <>
struct type_map_t<requested_i>
{
    using result_t = resolved_t;
};

namespace {

static_assert(std::is_same_v<resolved_t, mapped_type_t<resolved_t>>);
static_assert(std::is_same_v<resolved_t&, mapped_type_t<resolved_t&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<resolved_t&&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<resolved_t const>>);
static_assert(std::is_same_v<resolved_t&, mapped_type_t<resolved_t const&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<resolved_t const&&>>);

static_assert(std::is_same_v<resolved_t, mapped_type_t<requested_i>>);
static_assert(std::is_same_v<resolved_t&, mapped_type_t<requested_i&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<requested_i&&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<requested_i const>>);
static_assert(std::is_same_v<resolved_t&, mapped_type_t<requested_i const&>>);
static_assert(std::is_same_v<resolved_t, mapped_type_t<requested_i const&&>>);

} // namespace
} // namespace dink
