/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "ctor_factory.hpp"
#include <dink/test.hpp>
#include <string>

namespace dink {
namespace {

namespace ctor_factory {

// test contents of type after construction
namespace constexpr_ctor_test {

struct composite_t
{
    int_t int_val{};
    std::string string_val{};
    constexpr composite_t(int_t int_val, std::string string_val) : int_val{int_val}, string_val{string_val} {}
};

static_assert(ctor_factory_t<composite_t>{}(10, "10").int_val == 10);
static_assert(ctor_factory_t<composite_t>{}(10, "10").string_val == "10");

} // namespace constexpr_ctor_test

// test requires clause limits operator () to valid ctor args
namespace requires_clasus_limits_call_to_valid_ctor_args_test {

struct constructible_t
{
    constructible_t(int_t) {}
};

using factory_type_t = ctor_factory_t<constructible_t> const;

// valid call
static_assert(std::is_invocable_v<factory_type_t, int>, "Constraint should allow construction with valid arguments.");

// invalid call
static_assert(
    !std::is_invocable_v<factory_type_t, char const*>,
    "Constraint should prevent construction with invalid argument types."
);

// invalid call
static_assert(
    !std::is_invocable_v<factory_type_t, int, int>,
    "Constraint should prevent construction with incorrect argument count."
);

} // namespace requires_clasus_limits_call_to_valid_ctor_args_test

// test perfect forwarding selects correct ctor
TEST(ctor_factory_test, perfect_forwarding_selects_correct_ctor)
{
    struct forwarding_tester_t
    {
        enum class selected_ctor_t
        {
            none,
            lvalue_ref,
            rvalue_ref
        };
        selected_ctor_t selected_ctor = selected_ctor_t::none;

        forwarding_tester_t(std::string const&) { selected_ctor = selected_ctor_t::lvalue_ref; }
        forwarding_tester_t(std::string&&) { selected_ctor = selected_ctor_t::rvalue_ref; }
    };

    ctor_factory_t<forwarding_tester_t> const factory;

    // lvalue: pass a named variable
    auto const lvalue_str = std::string{"hello"};
    ASSERT_EQ(factory(lvalue_str).selected_ctor, forwarding_tester_t::selected_ctor_t::lvalue_ref);

    // rvalue: pass a temporary
    ASSERT_EQ(factory(std::string("world")).selected_ctor, forwarding_tester_t::selected_ctor_t::rvalue_ref);
}

// verify factory can construct objects whose constructors have move-only arguments
TEST(ctor_factory_test, can_construct_from_move_only_args)
{
    struct move_only_t
    {
        std::unique_ptr<int_t> ptr_val;
        explicit move_only_t(std::unique_ptr<int_t> p) : ptr_val{std::move(p)} {}
    };

    auto const expected_contents = int_t{5};
    auto source_ptr = std::make_unique<int_t>(expected_contents);

    ctor_factory_t<move_only_t> const factory;
    auto const widget = factory(std::move(source_ptr));

    ASSERT_EQ(source_ptr, nullptr);
    ASSERT_NE(widget.ptr_val, nullptr);
    ASSERT_EQ(*widget.ptr_val, expected_contents);
}

} // namespace ctor_factory
} // namespace
} // namespace dink
