/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "construct_in_allocation.hpp"
#include <dink/test.hpp>

#include <stdexcept>
#include <string>

namespace dink {
namespace {

struct construct_in_allocation_test_t : Test
{
    struct mock_allocation_deleter_t
    {
        MOCK_METHOD(void, call, (void* allocation), (const, noexcept));
    };
    StrictMock<mock_allocation_deleter_t> mock_deleter;

    struct allocation_deleter_t
    {
        auto operator()(void* allocation) const noexcept -> void { mock->call(allocation); }

        mock_allocation_deleter_t* mock = nullptr;
    };

    template <typename dst_t>
    struct dst_deleter_t
    {
        void operator()(dst_t* p) const { allocation_deleter(p); }

        [[no_unique_address]] allocation_deleter_t allocation_deleter;
    };

    struct simple_t
    {
        inline static auto const default_value = int_t{3};
        int_t value = default_value;
    };

    struct composite_t
    {
        inline static auto const default_int = int_t{5};
        inline static auto const default_string = std::string{"default"};
        int_t int_value = default_int;
        std::string string_value = default_string;
        composite_t(int_t int_value, std::string string_value)
            : int_value{int_value}, string_value{std::move(string_value)}
        {}
        composite_t() = default;
    };

    struct throwing_t
    {
        [[noreturn]] throwing_t() { throw std::runtime_error("threw from ctor"); }
    };

    using allocation_t = std::unique_ptr<void, allocation_deleter_t>;

    std::array<std::byte, 1024> expected_allocation_storage;
    void* expected_allocation = expected_allocation_storage.data();
};

TEST_F(construct_in_allocation_test_t, handles_nullptr)
{
    auto const result = construct_in_allocation<simple_t, dst_deleter_t<simple_t>>(
        allocation_t{nullptr, allocation_deleter_t{&mock_deleter}}
    );

    ASSERT_EQ(result, nullptr);
}

TEST_F(construct_in_allocation_test_t, constructs_object_and_transfers_deleter)
{
    auto const result = construct_in_allocation<simple_t, dst_deleter_t<simple_t>>(
        allocation_t{expected_allocation, allocation_deleter_t{&mock_deleter}}
    );

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->value, simple_t::default_value);

    EXPECT_CALL(mock_deleter, call(expected_allocation));
}

TEST_F(construct_in_allocation_test_t, forwards_ctor_args)
{
    auto const expected_int = composite_t::default_int + 1;
    auto const expected_string = std::string{"expected"};

    auto result = construct_in_allocation<composite_t, dst_deleter_t<composite_t>>(
        allocation_t{expected_allocation, allocation_deleter_t{&mock_deleter}}, expected_int, expected_string
    );

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->int_value, expected_int);
    EXPECT_EQ(result->string_value, expected_string);

    EXPECT_CALL(mock_deleter, call(expected_allocation));
}

TEST_F(construct_in_allocation_test_t, handles_throw_from_ctor)
{
    EXPECT_CALL(mock_deleter, call(expected_allocation));

    EXPECT_THROW(
        (void)(construct_in_allocation<throwing_t, dst_deleter_t<throwing_t>>(
            allocation_t{expected_allocation, allocation_deleter_t{&mock_deleter}}
        )),
        std::runtime_error
    );
}

} // namespace
} // namespace dink
