/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "arg.hpp"
#include <dink/test.hpp>
#include <type_traits>

namespace dink {
namespace {

// tests that sut deduces to the expected type for types we expect
struct test_arg_deduces_type_t
{
    struct resolver_t;

    struct deduced_t
    {};

    static constexpr auto const deduced_array_size = int{4};

    template <typename expected_arg_t>
    struct result_t;

    struct handler_t
    {
        template <typename expected_arg_t>
        constexpr auto handle(expected_arg_t expected_arg) const -> result_t<expected_arg_t>;
    };

    static constexpr handler_t handler{};

    template <typename sut_t, typename expected_type_t>
    constexpr auto test_deduction() -> void
    {
        static_assert(
            std::is_same_v<
                decltype(handler.template handle<expected_type_t>(std::declval<sut_t>())), result_t<expected_type_t>>
        );
    }

    template <typename sut_t>
    constexpr auto test_all_deductions() -> void
    {
        test_deduction<sut_t, deduced_t>();
        test_deduction<sut_t, deduced_t&>();
        test_deduction<sut_t, deduced_t&&>();
        test_deduction<sut_t, deduced_t const&>();
        test_deduction<sut_t, deduced_t const&&>();
        test_deduction<sut_t, deduced_t*>();
        test_deduction<sut_t, deduced_t const*>();
        test_deduction<sut_t, deduced_t*&>();
        test_deduction<sut_t, deduced_t const*&>();
        test_deduction<sut_t, deduced_t*&&>();
        test_deduction<sut_t, deduced_t const*&&>();
        test_deduction<sut_t, deduced_t* const&>();
        test_deduction<sut_t, deduced_t const* const&>();
        test_deduction<sut_t, deduced_t* const&&>();
        test_deduction<sut_t, deduced_t const* const&&>();
        test_deduction<sut_t, deduced_t(&)[deduced_array_size]>();
    }

    constexpr auto test_all_deductions_for_all_sut_types() -> void
    {
        using arg_t = arg_t<resolver_t, type_list_t<>>;
        test_all_deductions<arg_t>();
        test_all_deductions<single_arg_t<handler_t, arg_t>>();
    }

    constexpr test_arg_deduces_type_t() { test_all_deductions_for_all_sut_types(); }
};
constexpr auto test_arg_deduces_type = test_arg_deduces_type_t{};

// tests that arg_t updates the dependency chain correctly
struct test_arg_appends_canonical_deduced_to_dependency_chain_t
{
    struct deduced_t
    {};

    using initial_dependency_chain_t = type_list_t<int, void*>;
    using expected_dependency_chain_t = type_list_t<int, void*, deduced_t>;

    struct resolver_t
    {
        template <typename actual_deduced_t, typename next_dependency_chain_t>
        constexpr auto resolve() -> actual_deduced_t
        {
            static_assert(std::is_same_v<expected_dependency_chain_t, next_dependency_chain_t>);
            return actual_deduced_t{};
        }
    };

    template <typename actual_deduced_t>
    struct handler_t
    {
        constexpr auto handle(actual_deduced_t) const -> void {}
    };

    template <typename actual_deduced_t>
    constexpr auto test() -> void
    {
        auto resolver = resolver_t{};
        handler_t<actual_deduced_t>{}.handle(arg_t<resolver_t, initial_dependency_chain_t>{resolver});
        handler_t<actual_deduced_t*>{}.handle(arg_t<resolver_t, initial_dependency_chain_t>{resolver});
        handler_t<actual_deduced_t&&>{}.handle(arg_t<resolver_t, initial_dependency_chain_t>{resolver});
        handler_t<std::unique_ptr<actual_deduced_t>>{}.handle(arg_t<resolver_t, initial_dependency_chain_t>{resolver});
    }

    constexpr test_arg_appends_canonical_deduced_to_dependency_chain_t() { test<deduced_t>(); }
};
constexpr auto test_arg_appends_canonical_deduced_to_dependency_chain
    = test_arg_appends_canonical_deduced_to_dependency_chain_t{};

// tests that single_arg_t does not match copy or move ctors
struct test_single_arg_does_not_match_smf_t
{
    struct resolved_t
    {};

    struct other_t
    {};

    struct arg_t
    {
        operator resolved_t() const { return {}; }
        operator other_t() const { return {}; }
    };

    using sut_t = single_arg_t<resolved_t, arg_t>;

    static_assert(!std::is_constructible_v<resolved_t, sut_t const&>);
    static_assert(!std::is_constructible_v<resolved_t, sut_t&&>);
    static_assert(std::is_constructible_v<other_t, sut_t const&>);
};

} // namespace
} // namespace dink
