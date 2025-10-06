/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "instance_cache.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

#if 0
struct instance_cache_test_t : Test
{
    struct key_t
    {};

    using sut_t = instance_cache_t;
    sut_t sut{};
};

TEST_F(instance_cache_test_t, try_find_initially_empty)
{
    ASSERT_EQ(nullptr, (sut.try_find<key_t, int_t>()));
}

TEST_F(instance_cache_test_t, emplace_correctly_forwards_arguments)
{
    struct ctor_params_t
    {
        int_t integer;
        void* pointer;
        std::string moved_string;

        ctor_params_t(int_t integer, void* pointer, std::string&& string) noexcept
            : integer{integer}, pointer{pointer}, moved_string{std::move(string)}
        {}
    };

    auto const expected_integer = int_t{3};
    auto const expected_pointer = static_cast<void*>(this);
    auto const expected_string = std::string{"expected_string"};

    auto& actual = sut.emplace<key_t, ctor_params_t>(expected_integer, expected_pointer, std::string{expected_string});

    ASSERT_EQ(expected_integer, actual.integer);
    ASSERT_EQ(expected_pointer, actual.pointer);
    ASSERT_EQ(expected_string, actual.moved_string);
}

TEST_F(instance_cache_test_t, emplace_returns_expected_value)
{
    auto const expected = int_t{3};

    auto const actual = sut.emplace<key_t, int_t>(expected);

    ASSERT_EQ(expected, actual);
}

TEST_F(instance_cache_test_t, emplace_returns_unique_addresses_per_type)
{
    auto const& actual_int = sut.emplace<int_t, int_t>(3);
    auto const& actual_ptr = sut.emplace<void*, void*>(this);

    ASSERT_NE(static_cast<void const*>(&actual_int), static_cast<void const*>(&actual_ptr));
}

TEST_F(instance_cache_test_t, address_returned_from_emplace_matches_address_returned_from_find)
{
    auto& expected = sut.emplace<key_t, int_t>(3);

    auto* const result = sut.try_find<key_t, int_t>();

    ASSERT_EQ(&expected, result);
}

TEST_F(instance_cache_test_t, nonconst_find_matches_const_find)
{
    sut.emplace<key_t, int_t>(3);

    auto const expected = sut.try_find<key_t, int_t>();
    auto const actual = static_cast<sut_t const&>(sut).try_find<key_t, int_t>();

    ASSERT_EQ(expected, actual);
}
#endif

} // namespace
} // namespace dink
