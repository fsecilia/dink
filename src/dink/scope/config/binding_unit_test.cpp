/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "binding.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct keys_test_t : Test
{
    template <int id>
    struct unique_key_t
    {};

    template <typename key_p>
    struct element_t
    {
        using key_t = key_p;
    };
};

TEST_F(keys_test_t, zero_elements)
{
    using expected_tuple_t = std::tuple<>;
    using input_tuple_t = std::tuple<>;

    using actual_tuple_t = keys_t<input_tuple_t>;

    static_assert(std::same_as<expected_tuple_t, actual_tuple_t>);
}

TEST_F(keys_test_t, single_element)
{
    using expected_tuple_t = std::tuple<unique_key_t<0>>;
    using input_tuple_t = std::tuple<element_t<std::tuple_element_t<0, expected_tuple_t>>>;
    using actual_tuple_t = keys_t<input_tuple_t>;

    static_assert(std::same_as<expected_tuple_t, actual_tuple_t>);
}

TEST_F(keys_test_t, multiple_elements)
{
    using expected_tuple_t = std::tuple<unique_key_t<0>, unique_key_t<1>, unique_key_t<2>>;
    using input_tuple_t = std::tuple<
        element_t<std::tuple_element_t<0, expected_tuple_t>>, element_t<std::tuple_element_t<1, expected_tuple_t>>,
        element_t<std::tuple_element_t<2, expected_tuple_t>>>;

    using actual_tuple_t = keys_t<input_tuple_t>;

    static_assert(std::same_as<expected_tuple_t, actual_tuple_t>);
}

} // namespace
} // namespace dink
