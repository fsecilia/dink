/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/arg.hpp>

namespace dink {
namespace {

struct fixture_t
{
    using id_t = int_t;
    static constexpr auto const unexpected_id = id_t{123};
    static constexpr auto const expected_id = id_t{456};

    struct deduced_t
    {
        id_t id = unexpected_id;

        // this type must have at least a copy ctor to test single_arg_t, though a move ctor is also useful
        deduced_t(id_t id) noexcept : id{id} {}
        deduced_t(deduced_t const&) = default;
        deduced_t(deduced_t&&) = default;
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

        auto dispatch_resolve(tag_t<id_t>) -> id_t { return resolve_id(); }
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

        MOCK_METHOD(id_t, resolve_id, (), ());
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

    auto expect_id() noexcept -> void { EXPECT_CALL(container, resolve_id()).WillOnce(Return(expected_id)); }
    auto expect_val() noexcept -> void { EXPECT_CALL(container, resolve_val()).WillOnce(Return(deduced)); }
    auto expect_ref() noexcept -> void { EXPECT_CALL(container, resolve_ref()).WillOnce(ReturnRef(deduced)); }
    auto expect_ptr() noexcept -> void { EXPECT_CALL(container, resolve_ptr()).WillOnce(Return(deduced_ptr)); }

    auto expect_ptr_ref() noexcept -> void
    {
        EXPECT_CALL(container, resolve_ptr_ref()).WillOnce(ReturnRef(deduced_ptr));
    }

    auto expect_cptr_ref() noexcept -> void
    {
        EXPECT_CALL(container, resolve_cptr_ref()).WillOnce(ReturnRef(deduced_cptr));
    }

    auto expect_arr_ref() noexcept -> void
    {
        EXPECT_CALL(container, resolve_arr_ref()).WillOnce(ReturnRef(deduced_array));
    }
};

template <typename dispatcher_t>
struct arg_test_t : fixture_t, Test
{
    using sut_t = typename dispatcher_t::sut_t;
    sut_t sut = dispatcher_t{}.create_sut(fixture_t::container);

    auto test_val() noexcept -> void
    {
        expect_val();
        ASSERT_EQ(expected_id, handler.val(sut));
    }

    auto test_lref() noexcept -> void
    {
        expect_ref();
        ASSERT_EQ(expected_id, handler.lref(sut));
    }

    auto test_rref() noexcept -> void
    {
        expect_val();
        ASSERT_EQ(expected_id, handler.rref(sut));
    }

    auto test_lcref() noexcept -> void
    {
        expect_ref();
        ASSERT_EQ(expected_id, handler.lcref(sut));
    }

    auto test_rcref() noexcept -> void
    {
        expect_val();
        ASSERT_EQ(expected_id, handler.rcref(sut));
    }

    auto test_ptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.ptr(sut));
    }

    auto test_cptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.cptr(sut));
    }

    auto test_lrefptr() noexcept -> void
    {
        expect_ptr_ref();
        ASSERT_EQ(expected_id, handler.lrefptr(sut));
    }

    auto test_lrefcptr() noexcept -> void
    {
        expect_cptr_ref();
        ASSERT_EQ(expected_id, handler.lrefcptr(sut));
    }

    auto test_rrefptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.rrefptr(sut));
    }

    auto test_rrefcptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.rrefcptr(sut));
    }

    auto test_lcrefptr() noexcept -> void
    {
        expect_ptr_ref();
        ASSERT_EQ(expected_id, handler.lcrefptr(sut));
    }

    auto test_lcrefcptr() noexcept -> void
    {
        expect_ptr_ref();
        ASSERT_EQ(expected_id, handler.lcrefcptr(sut));
    }

    auto test_rcrefptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.rcrefptr(sut));
    }

    auto test_rcrefcptr() noexcept -> void
    {
        expect_ptr();
        ASSERT_EQ(expected_id, handler.rcrefcptr(sut));
    }

    auto test_arr_ref() noexcept -> void
    {
        expect_arr_ref();
        ASSERT_EQ(expected_id + deduced_array_size, handler.arr_ref(sut));
    }
};

struct arg_test_dispatcher_t
{
    using sut_t = arg_t<fixture_t::container_t>;
    auto create_sut(fixture_t::container_t& container) const noexcept -> sut_t { return sut_t{container}; }
};

struct single_arg_test_dispatcher_t
{
    using sut_t = single_arg_t<fixture_t::handler_t, arg_t<fixture_t::container_t>>;
    auto create_sut(fixture_t::container_t& container) const noexcept -> sut_t
    {
        return sut_t{arg_t<fixture_t::container_t>{container}};
    }
};

using types_t = ::testing::Types<arg_test_dispatcher_t, single_arg_test_dispatcher_t>;
TYPED_TEST_SUITE(arg_test_t, types_t);

TYPED_TEST(arg_test_t, val)
{
    this->test_val();
}

TYPED_TEST(arg_test_t, lref)
{
    this->test_lref();
}

TYPED_TEST(arg_test_t, rref)
{
    this->test_rref();
}

TYPED_TEST(arg_test_t, lcref)
{
    this->test_lcref();
}

TYPED_TEST(arg_test_t, rcref)
{
    this->test_rcref();
}

TYPED_TEST(arg_test_t, ptr)
{
    this->test_ptr();
}

TYPED_TEST(arg_test_t, cptr)
{
    this->test_cptr();
}

TYPED_TEST(arg_test_t, lrefptr)
{
    this->test_lrefptr();
}

TYPED_TEST(arg_test_t, lrefcptr)
{
    this->test_lrefcptr();
}

TYPED_TEST(arg_test_t, rrefptr)
{
    this->test_rrefptr();
}

TYPED_TEST(arg_test_t, rrefcptr)
{
    this->test_rrefcptr();
}

TYPED_TEST(arg_test_t, lcrefptr)
{
    this->test_lcrefptr();
}

TYPED_TEST(arg_test_t, lcrefcptr)
{
    this->test_lcrefcptr();
}

TYPED_TEST(arg_test_t, rcrefptr)
{
    this->test_rcrefptr();
}

TYPED_TEST(arg_test_t, rcrefcptr)
{
    this->test_rcrefcptr();
}

TYPED_TEST(arg_test_t, arr_ref)
{
    this->test_arr_ref();
}

// ---------------------------------------------------------------------------------------------------------------------

/*
    The typed tests above pass arg_t or single_arg_t to named methods in handler_t to check they deduce parameters
    correctly and return a properly-typed instance of deduced_t. This test tries to instantiate deduced_t itself
    directly calling its single-argument ctor with arg_t or single_arg_t. The behavior is very different between them.
*/
struct single_arg_test_t : fixture_t, Test
{};

TEST_F(single_arg_test_t, arg_matches_smf_ctor)
{
    using sut_t = arg_t<fixture_t::container_t>;
    sut_t sut = sut_t{container};

    // arg_t matches deduced_t's move ctor first, copy ctor second, id ctor third, but this is not useful
    expect_val();
    auto result = deduced_t{sut};
    ASSERT_EQ(expected_id, result.id);
}

TEST_F(single_arg_test_t, single_arg_does_not_match_smf)
{
    using sut_t = single_arg_t<deduced_t, arg_t<fixture_t::container_t>>;
    sut_t sut = sut_t{arg_t<fixture_t::container_t>{container}};

    // single_arg_t prevents matching the smf ctors, correctly selecting the id_t ctor instead
    expect_id();
    auto result = deduced_t{sut};
    ASSERT_EQ(expected_id, result.id);
}

} // namespace
} // namespace dink
