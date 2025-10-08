/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "factory_invoker.hpp"
#include <dink/test.hpp>

namespace dink {

namespace factory_invoker::detail {
namespace {

struct factory_invoker_detail_test_t : Test
{
    struct resolver_t
    {};

    struct arg_t
    {
        resolver_t& resolver;
    };

    struct single_arg_t
    {
        arg_t arg;
    };

    resolver_t resolver{};
};

// ---------------------------------------------------------------------------------------------------------------------

struct factory_invoker_detail_indexed_arg_factory_test_t : factory_invoker_detail_test_t
{
    using sut_t = indexed_arg_factory_t<arg_t, single_arg_t>;

    // when creating a single arg, the type is single_arg_t
    static_assert(std::is_same_v<single_arg_t, decltype(std::declval<sut_t>().template create<1, 0>(resolver))>);

    // when creating multiple args, the type is arg_t
    static_assert(std::is_same_v<arg_t, decltype(std::declval<sut_t>().template create<2, 0>(resolver))>);
    static_assert(std::is_same_v<arg_t, decltype(std::declval<sut_t>().template create<2, 1>(resolver))>);
};

TEST_F(factory_invoker_detail_indexed_arg_factory_test_t, arg_is_initialized_with_resolver)
{
    ASSERT_EQ(&resolver, &(sut_t{}.template create<2, 0>(resolver).resolver));
    ASSERT_EQ(&resolver, &(sut_t{}.template create<2, 1>(resolver).resolver));
}

TEST_F(factory_invoker_detail_indexed_arg_factory_test_t, single_arg_is_initialized_with_resolver)
{
    ASSERT_EQ(&resolver, &(sut_t{}.template create<1, 0>(resolver).arg.resolver));
}

// ---------------------------------------------------------------------------------------------------------------------

struct factory_invoker_detail_dispatcher_test_t : factory_invoker_detail_test_t
{
    struct constructed_t
    {
        std::size_t arity;
    };

    struct indexed_arg_factory_t
    {
        resolver_t* resolver = nullptr;

        template <std::size_t, std::size_t index>
        constexpr auto create(auto& resolver) -> auto
        {
            this->resolver = &resolver;
            return arg_t{resolver};
        }
    };

    struct instance_factory_t
    {
        resolver_t* resolver;

        auto assert_arg(arg_t const& arg) const noexcept -> void { ASSERT_EQ(resolver, &arg.resolver); }

        constexpr auto operator()() -> constructed_t { return constructed_t{0}; }

        constexpr auto operator()(arg_t const& arg) -> constructed_t
        {
            assert_arg(arg);
            return constructed_t{1};
        }

        constexpr auto operator()(arg_t const& arg1, arg_t const& arg2) -> constructed_t
        {
            assert_arg(arg1);
            assert_arg(arg2);
            return constructed_t{2};
        }
    };

    template <std::size_t arity>
    using sut_t = arity_dispatcher_t<constructed_t, indexed_arg_factory_t, std::make_index_sequence<arity>>;
};

TEST_F(factory_invoker_detail_dispatcher_test_t, arity_0_constructs_with_0_args)
{
    auto sut = sut_t<0>{};
    auto instance_factory = instance_factory_t{.resolver = &resolver};
    auto const constructed = sut(instance_factory, resolver);
    ASSERT_EQ(0, constructed.arity);
}

TEST_F(factory_invoker_detail_dispatcher_test_t, arity_1_constructs_with_1_args)
{
    auto sut = sut_t<1>{};
    auto instance_factory = instance_factory_t{.resolver = &resolver};
    auto const constructed = sut(instance_factory, resolver);
    ASSERT_EQ(1, constructed.arity);
}

TEST_F(factory_invoker_detail_dispatcher_test_t, arity_2_constructs_with_2_args)
{
    auto sut = sut_t<2>{};
    auto instance_factory = instance_factory_t{.resolver = &resolver};
    auto const constructed = sut(instance_factory, resolver);
    ASSERT_EQ(2, constructed.arity);
}

} // namespace
} // namespace factory_invoker::detail

// ---------------------------------------------------------------------------------------------------------------------

namespace {

struct factory_invoker_test_t : Test
{
    struct resolver_t
    {};
    resolver_t resolver;

    struct arg_t
    {
        constexpr explicit arg_t(resolver_t&) {}
    };

    struct single_arg_t
    {
        constexpr explicit single_arg_t(arg_t) {}
    };

    struct constructed_t
    {
        int_t arity = -1;
    };

    template <std::size_t arity>
    using sut_t = factory_invoker_t<constructed_t, arity, arg_t, single_arg_t>;
};

TEST_F(factory_invoker_test_t, invokes_factory_with_zero_args)
{
    auto factory = [&]() { return constructed_t{0}; };

    auto const result = sut_t<0>{}(factory, resolver);

    ASSERT_EQ(result.arity, 0);
}

TEST_F(factory_invoker_test_t, invokes_factory_with_single_arg)
{
    auto factory = [&](single_arg_t const&) { return constructed_t{1}; };

    auto const result = sut_t<1>{}(factory, resolver);

    ASSERT_EQ(result.arity, 1);
}

TEST_F(factory_invoker_test_t, invokes_factory_with_multiple_args)
{
    auto factory = [&](arg_t const&, arg_t const&) { return constructed_t{2}; };

    auto const result = sut_t<2>{}(factory, resolver);

    ASSERT_EQ(result.arity, 2);
}

} // namespace
} // namespace dink
