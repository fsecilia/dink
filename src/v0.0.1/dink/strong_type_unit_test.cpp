/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/strong_type.hpp>

namespace dink {
namespace {

using namespace testing;

struct strong_type_test_t : Test
{
    // simple value type sut used by most tests
    struct tag_t;
    using value_t = int_t;
    using sut_t = strong_type_t<value_t, tag_t>;
    static constexpr value_t const value{3};
    static constexpr sut_t sut{value};

    // compound value used to test member access and construct
    struct compound_value_t
    {
        int_t value;
        static auto construct(int_t value) -> compound_value_t { return compound_value_t{.value = value}; }
    };
    using compound_sut_t = strong_type_t<compound_value_t, struct compound_value_t>;
    static constexpr compound_sut_t compound_sut{compound_value_t{value}};

    // other is used for conversion
    struct other_tag_t;
    using other_value_t = float_t;
    using other_sut_t = strong_type_t<other_value_t, other_tag_t>;
    static constexpr other_value_t const other_value{5.0};
    static constexpr other_sut_t const other_sut{other_value};

    strong_type_test_t() noexcept
    {
        // operator value_t const&
        static_assert(value == static_cast<value_t>(static_cast<sut_t const&>(sut)));

        // operator value_t&
        static_assert(value == static_cast<value_t>(sut));

        // get const
        static_assert(value == static_cast<sut_t const&>(sut).get());

        // get
        static_assert(value == sut.get());

        // indirection const
        static_assert(&compound_sut.get() == &*static_cast<compound_sut_t const&>(compound_sut));

        // indirection
        static_assert(&compound_sut.get() == &*compound_sut);

        // member access const
        static_assert(&compound_sut.get().value == &static_cast<compound_sut_t const&>(compound_sut)->value);

        // member access
        static_assert(&compound_sut.get().value == &compound_sut->value);

        // default constructor
        static_assert(value_t{} == sut_t{}.get());

        // copy ctor
        static_assert(value == sut_t{sut}.get());

        // conversion copy ctor
        static_assert(static_cast<value_t>(other_value) == sut_t{other_sut}.get());
    }
};

TEST_F(strong_type_test_t, move_ctor)
{
    auto dst = sut_t{};
    ASSERT_NE(dst, sut);

    dst = std::move(sut);
    ASSERT_EQ(dst, sut);

    static_assert(value == sut_t{sut_t{sut}}.get());
}

TEST_F(strong_type_test_t, copy_assignment)
{
    auto dst = sut_t{};
    ASSERT_NE(dst, sut);

    dst = sut;
    ASSERT_EQ(dst, sut);
}

TEST_F(strong_type_test_t, move_assignment)
{
    auto dst = sut_t{};
    ASSERT_NE(dst, sut);

    auto src = sut;
    dst = std::move(src);
    ASSERT_EQ(dst, sut);
}

TEST_F(strong_type_test_t, conversion_assignment)
{
    auto dst = sut_t{};
    ASSERT_NE(dst.get(), sut.get());

    auto src = sut;
    dst = std::move(src);
    ASSERT_EQ(dst, sut);
}

TEST_F(strong_type_test_t, construct)
{
    auto sut = compound_sut_t::construct(value);
    ASSERT_EQ(value, sut.get().value);
}

} // namespace
} // namespace dink
