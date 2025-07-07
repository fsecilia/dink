/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/arg.hpp>
#include <dink/test.hpp>

namespace dink {
namespace {

struct arg_test_t : Test
{
    using id_t = int_t;

    static constexpr auto const unexpected_id = id_t{123};
    static constexpr auto const expected_id = id_t{456};

    struct resolved_t
    {
        id_t id = unexpected_id;
    };

    struct handler_t
    {
        auto val(resolved_t resolved) const noexcept -> id_t { return resolved.id; }
        auto lref(resolved_t& resolved) const noexcept -> id_t { return resolved.id; }
        auto rref(resolved_t&& resolved) const noexcept -> id_t { return resolved.id; }
        auto lcref(resolved_t const& resolved) const noexcept -> id_t { return resolved.id; }
        auto rcref(resolved_t const&& resolved) const noexcept -> id_t { return resolved.id; }
    };

    struct composer_t
    {
        template <typename resolved_t>
        struct tag_t
        {};

        template <typename resolved_t>
        auto resolve() -> decltype(auto)
        {
            return dispatch_resolve(tag_t<resolved_t>{});
        }

        auto dispatch_resolve(tag_t<resolved_t>) -> resolved_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<resolved_t&>) -> resolved_t& { return resolve_ref(); }
        auto dispatch_resolve(tag_t<resolved_t&&>) -> resolved_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<resolved_t const>) -> resolved_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<resolved_t const&>) -> resolved_t& { return resolve_ref(); }
        auto dispatch_resolve(tag_t<resolved_t const&&>) -> resolved_t { return resolve_val(); }

        MOCK_METHOD(resolved_t, resolve_val, (), ());
        MOCK_METHOD(resolved_t&, resolve_ref, (), ());

        virtual ~composer_t() = default;
    };

    auto expect_ref() -> void { EXPECT_CALL(composer, resolve_ref()).WillOnce(ReturnRef(resolved)); }
    auto expect_val() -> void { EXPECT_CALL(composer, resolve_val()).WillOnce(Return(resolved)); }

    using sut_t = arg_t<resolved_t, composer_t, 1>;

    resolved_t resolved{expected_id};
    handler_t handler{};
    StrictMock<composer_t> composer{};
    sut_t sut{composer};
};

TEST_F(arg_test_t, val)
{
    expect_val();
    ASSERT_EQ(expected_id, handler.val(sut));
}

TEST_F(arg_test_t, lref)
{
    expect_ref();
    ASSERT_EQ(expected_id, handler.lref(sut));
}

TEST_F(arg_test_t, rref)
{
    expect_val();
    ASSERT_EQ(expected_id, handler.rref(sut));
}

TEST_F(arg_test_t, lcref)
{
    expect_ref();
    ASSERT_EQ(expected_id, handler.lcref(sut));
}

TEST_F(arg_test_t, rcref)
{
    expect_val();
    ASSERT_EQ(expected_id, handler.rcref(sut));
}

} // namespace
} // namespace dink
