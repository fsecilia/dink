/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "instance_cache.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct cache_entry_test_t : Test
{
    using sut_t = cache_entry_t;

    struct mock_value_t
    {
        MOCK_METHOD(void, dtor, (), (noexcept));
        ~mock_value_t() = default;
    };
    StrictMock<mock_value_t> mock_value{};

    struct value_t
    {
        ~value_t() { mock->dtor(); }
        mock_value_t* mock = nullptr;
    };
};

TEST_F(cache_entry_test_t, empty_entry_is_safe_to_destroy)
{
    auto sut = sut_t{};
}

// ---------------------------------------------------------------------------------------------------------------------

struct cache_entry_test_empty_t : cache_entry_test_t
{
    sut_t sut;

    auto emplace_value() -> value_t& { return sut.emplace<value_t>(&mock_value); }
};

TEST_F(cache_entry_test_empty_t, has_value_returns_false)
{
    ASSERT_FALSE(sut.has_value());
}

TEST_F(cache_entry_test_empty_t, emplaced_value_is_destroyed_by_dtor)
{
    emplace_value();

    EXPECT_CALL(mock_value, dtor());
}

TEST_F(cache_entry_test_empty_t, emplace_correctly_forwards_arguments)
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

    auto result = sut.emplace<ctor_params_t>(expected_integer, expected_pointer, std::string{expected_string});

    ASSERT_EQ(expected_integer, result.integer);
    ASSERT_EQ(expected_pointer, result.pointer);
    ASSERT_EQ(expected_string, result.moved_string);
}

// ---------------------------------------------------------------------------------------------------------------------

struct cache_entry_test_populated_t : cache_entry_test_empty_t
{
    StrictMock<mock_value_t> new_mock_value{};
    value_t& value = emplace_value();
};

TEST_F(cache_entry_test_populated_t, has_value_returns_true)
{
    ASSERT_TRUE(sut.has_value());

    EXPECT_CALL(mock_value, dtor());
}

TEST_F(cache_entry_test_populated_t, value_returned_from_emplace_matches_get_as)
{
    ASSERT_EQ(&value, &sut.get_as<value_t>());

    EXPECT_CALL(mock_value, dtor());
}

TEST_F(cache_entry_test_populated_t, replacing_via_emplace_destroys_previous_value_immediately)
{
    struct new_value_t
    {};

    EXPECT_CALL(mock_value, dtor());
    sut.emplace<new_value_t>();
}

// ---------------------------------------------------------------------------------------------------------------------

struct cache_entry_test_populated_for_replacement_t : cache_entry_test_empty_t
{
    StrictMock<mock_value_t> new_mock_value{};
    sut_t sut;

    value_t& value = emplace_value();
};

TEST_F(cache_entry_test_populated_for_replacement_t, replacing_via_emplace_destroys_new_value_from_dtor)
{
    struct new_value_t
    {
        ~new_value_t() { mock->dtor(); }
        mock_value_t* mock = nullptr;
    };

    EXPECT_CALL(mock_value, dtor());
    sut.emplace<new_value_t>(&new_mock_value);

    EXPECT_CALL(new_mock_value, dtor());
}

} // namespace
} // namespace dink
