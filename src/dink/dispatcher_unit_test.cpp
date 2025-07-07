/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <compare>
#include <dink/dispatcher.hpp>
#include <dink/test.hpp>

namespace dink {
namespace {

struct dispatcher_test_t : Test
{
    struct composer_t
    {};

    template <typename resolved_t, typename composer_t, int_t num_args>
    struct arg_t
    {
        composer_t const* composer = nullptr;
        operator composer_t const*() const noexcept { return composer; }
        operator int_t() const noexcept { return reinterpret_cast<int_t>(composer); }
        explicit arg_t(composer_t& composer) noexcept : composer{&composer} {}
    };

    template <typename resolved_t>
    struct factory_t
    {
        static_assert(!std::is_aggregate_v<resolved_t>);

        template <typename... args_t>
        requires(std::is_constructible_v<resolved_t, args_t...>)
        constexpr auto operator()(args_t&&... args) const -> resolved_t
        {
            return resolved_t{std::forward<args_t>(args)...};
        }
    };

    composer_t composer{};
};

TEST_F(dispatcher_test_t, resolve_0_arg)
{
    constexpr auto const expected_default_value = int_t{3};
    struct resolved_t
    {
        int_t actual_default_value{expected_default_value};
        resolved_t() = default;
    };

    using sut_t = dispatcher_t<resolved_t, composer_t, factory_t<resolved_t>, arg_t>;
    sut_t sut{};

    auto const resolved = sut(composer);
    ASSERT_EQ(expected_default_value, resolved.actual_default_value);
}

TEST_F(dispatcher_test_t, resolve_1_arg)
{
    struct resolved_t
    {
        int_t composer;

        resolved_t(int_t composer) : composer{composer} {}
    };

    using sut_t = dispatcher_t<resolved_t, composer_t, factory_t<resolved_t>, arg_t>;
    sut_t sut{};

    auto const resolved = sut(composer);
    ASSERT_EQ(reinterpret_cast<int_t>(&composer), resolved.composer);
}

TEST_F(dispatcher_test_t, resolve_2_arg)
{
    struct resolved_t
    {
        int_t composer0;
        composer_t const* composer1;

        resolved_t(int_t composer0, composer_t const* composer1) : composer0{composer0}, composer1{composer1} {}
    };

    using sut_t = dispatcher_t<resolved_t, composer_t, factory_t<resolved_t>, arg_t>;
    sut_t sut{};

    auto const resolved = sut(composer);
    ASSERT_EQ(reinterpret_cast<int_t>(&composer), resolved.composer0);
    ASSERT_EQ(&composer, resolved.composer1);
}

} // namespace
} // namespace dink
