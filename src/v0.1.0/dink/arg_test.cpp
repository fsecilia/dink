/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/arg.hpp>

namespace dink {
namespace {

struct arg_test_t : Test
{
    using id_t = int_t;
    static constexpr auto const unexpected_id = id_t{123};
    static constexpr auto const expected_id = id_t{456};

    struct deduced_t
    {
        id_t id = unexpected_id;
    };
    deduced_t deduced{expected_id};
    deduced_t* deduced_ptr{&deduced};
    deduced_t const* deduced_cptr{&deduced};

    static constexpr auto const deduced_array_size = int{4};
    deduced_t deduced_array[deduced_array_size]{expected_id + 1, expected_id + 2, expected_id + 3, expected_id + 4};
    deduced_t (&deduced_array_ref)[deduced_array_size] = deduced_array;

    struct handler_t
    {
        auto val(deduced_t deduced) const noexcept -> id_t { return deduced.id; }
        auto lref(deduced_t& deduced) const noexcept -> id_t { return deduced.id; }
        auto rref(deduced_t&& deduced) const noexcept -> id_t { return deduced.id; }
        auto lcref(deduced_t const& deduced) const noexcept -> id_t { return deduced.id; }
        auto rcref(deduced_t const&& deduced) const noexcept -> id_t { return deduced.id; }
        auto ptr(deduced_t* deduced) const noexcept -> id_t { return deduced->id; }
        auto cptr(deduced_t const* deduced) const noexcept -> id_t { return deduced->id; }
        auto lrefptr(deduced_t*& deduced) const noexcept -> id_t { return deduced->id; }
        auto lrefcptr(deduced_t const*& deduced) const noexcept -> id_t { return deduced->id; }
        auto rrefptr(deduced_t*&& deduced) const noexcept -> id_t { return deduced->id; }
        auto rrefcptr(deduced_t const*&& deduced) const noexcept -> id_t { return deduced->id; }
        auto lcrefptr(deduced_t* const& deduced) const noexcept -> id_t { return deduced->id; }
        auto lcrefcptr(deduced_t const* const& deduced) const noexcept -> id_t { return deduced->id; }
        auto rcrefptr(deduced_t* const&& deduced) const noexcept -> id_t { return deduced->id; }
        auto rcrefcptr(deduced_t const* const&& deduced) const noexcept -> id_t { return deduced->id; }
        auto arr_ref(deduced_t (&deduced)[deduced_array_size]) const noexcept -> id_t
        {
            return deduced[deduced_array_size - 1].id;
        }
    };
    handler_t handler{};

    struct container_t
    {
        // we capture the exact deduced type in a tag so we can dispatch without decay
        template <typename deduced_t>
        struct tag_t
        {};

        template <typename deduced_t>
        auto resolve() -> decltype(auto)
        {
            return dispatch_resolve(tag_t<deduced_t>{});
        }

        auto dispatch_resolve(tag_t<deduced_t>) -> deduced_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<deduced_t&>) -> deduced_t& { return resolve_ref(); }
        auto dispatch_resolve(tag_t<deduced_t&&>) -> deduced_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<deduced_t const>) -> deduced_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<deduced_t const&>) -> deduced_t& { return resolve_ref(); }
        auto dispatch_resolve(tag_t<deduced_t const&&>) -> deduced_t { return resolve_val(); }
        auto dispatch_resolve(tag_t<deduced_t*>) -> deduced_t* { return resolve_ptr(); }
        auto dispatch_resolve(tag_t<deduced_t const*>) -> deduced_t const* { return resolve_ptr(); }
        auto dispatch_resolve(tag_t<deduced_t*&>) -> deduced_t*& { return resolve_ptr_ref(); }
        auto dispatch_resolve(tag_t<deduced_t const*&>) -> deduced_t const*& { return resolve_cptr_ref(); }
        auto dispatch_resolve(tag_t<deduced_t* const&>) -> deduced_t* const& { return resolve_ptr_ref(); }
        auto dispatch_resolve(tag_t<deduced_t const* const&>) -> deduced_t const* const& { return resolve_ptr_ref(); }

        template <typename deduced_t, int size>
        auto dispatch_resolve(tag_t<deduced_t (&)[size]>) -> deduced_t (&)[size]
        {
            return resolve_arr_ref();
        }

        MOCK_METHOD(deduced_t, resolve_val, (), ());
        MOCK_METHOD(deduced_t&, resolve_ref, (), ());
        MOCK_METHOD(deduced_t*, resolve_ptr, (), ());
        MOCK_METHOD(deduced_t*&, resolve_ptr_ref, (), ());
        MOCK_METHOD(deduced_t*&&, resolve_ptr_rref, (), ());
        MOCK_METHOD(deduced_t const*&, resolve_cptr_ref, (), ());
        MOCK_METHOD(deduced_t const*&&, resolve_cptr_rref, (), ());
        MOCK_METHOD(deduced_t (&)[deduced_array_size], resolve_arr_ref, (), ());

        virtual ~container_t() = default;
    };
    StrictMock<container_t> container{};

    auto expect_ref() -> void { EXPECT_CALL(container, resolve_ref()).WillOnce(ReturnRef(deduced)); }
    auto expect_val() -> void { EXPECT_CALL(container, resolve_val()).WillOnce(Return(deduced)); }
    auto expect_ptr() -> void { EXPECT_CALL(container, resolve_ptr()).WillOnce(Return(deduced_ptr)); }
    auto expect_ptr_ref() -> void { EXPECT_CALL(container, resolve_ptr_ref()).WillOnce(ReturnRef(deduced_ptr)); }
    auto expect_cptr_ref() -> void { EXPECT_CALL(container, resolve_cptr_ref()).WillOnce(ReturnRef(deduced_cptr)); }
    auto expect_arr_ref() -> void { EXPECT_CALL(container, resolve_arr_ref()).WillOnce(ReturnRef(deduced_array)); }

    using sut_t = arg_t<container_t>;
    sut_t sut{container};
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

TEST_F(arg_test_t, ptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.ptr(sut));
}

TEST_F(arg_test_t, cptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.cptr(sut));
}

TEST_F(arg_test_t, lrefptr)
{
    expect_ptr_ref();
    ASSERT_EQ(expected_id, handler.lrefptr(sut));
}

TEST_F(arg_test_t, lrefcptr)
{
    expect_cptr_ref();
    ASSERT_EQ(expected_id, handler.lrefcptr(sut));
}

TEST_F(arg_test_t, rrefptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.rrefptr(sut));
}

TEST_F(arg_test_t, rrefcptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.rrefcptr(sut));
}

TEST_F(arg_test_t, lcrefptr)
{
    expect_ptr_ref();
    ASSERT_EQ(expected_id, handler.lcrefptr(sut));
}

TEST_F(arg_test_t, lcrefcptr)
{
    expect_ptr_ref();
    ASSERT_EQ(expected_id, handler.lcrefcptr(sut));
}

TEST_F(arg_test_t, rcrefptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.rcrefptr(sut));
}

TEST_F(arg_test_t, rcrefcptr)
{
    expect_ptr();
    ASSERT_EQ(expected_id, handler.rcrefcptr(sut));
}

TEST_F(arg_test_t, arr_ref)
{
    expect_arr_ref();
    ASSERT_EQ(expected_id + deduced_array_size, handler.arr_ref(sut));
}

} // namespace
} // namespace dink
