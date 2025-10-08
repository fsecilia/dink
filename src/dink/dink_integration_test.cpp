/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/bindings.hpp>
#include <dink/container.hpp>

namespace dink {
namespace {

inline static int_t next_id = 1;

template <typename... args_t>
struct constructed_from_t
{
    int_t id = next_id++;
    constructed_from_t(args_t...) noexcept {}
};

struct integration_smoke_test_t : Test
{};

TEST_F(integration_smoke_test_t, default_ctor_transient)
{
    using instance_t = constructed_from_t<>;

    auto container = root_container_t<>{};
    auto a1 = container.resolve<instance_t>();
    auto a2 = container.resolve<instance_t>();

    ASSERT_NE(a1.id, a2.id); // Transient - different instances
}

TEST_F(integration_smoke_test_t, default_ctor_singleton)
{
    using instance_t = constructed_from_t<>;

    auto container = root_container_t<>{};
    auto& a1 = container.resolve<instance_t&>();
    auto& a2 = container.resolve<instance_t&>();

    ASSERT_EQ(&a1, &a2); // Singleton - same instance
}

TEST_F(integration_smoke_test_t, dependency_graph)
{
    using instance_t = constructed_from_t<constructed_from_t<>, constructed_from_t<constructed_from_t<>>>;

    auto container = root_container_t<>{};
    auto c = container.resolve<instance_t>();

    EXPECT_TRUE(c.id > 0); // Successfully constructed with dependencies
}

TEST_F(integration_smoke_test_t, explicit_binding)
{
    using instance_t = constructed_from_t<>;

    auto container = root_container_t{bind<instance_t>().to<instance_t>().in<scopes::singleton_t>()};
    auto a1 = container.resolve<instance_t>();
    auto a2 = container.resolve<instance_t>();

    ASSERT_EQ(a1.id, a2.id); // Explicitly bound singleton
}

TEST_F(integration_smoke_test_t, circular_dependency_detection)
{
    struct A;
    struct B;

    struct A
    {
        A(B const&) {}
    };
    struct B
    {
        B(A const&) {}
    };

    auto container = root_container_t<>{};

    container.resolve<A>();

    // Force full instantiation via sizeof
    constexpr bool can_resolve_a = requires { sizeof(decltype(container.resolve<A>())); };
    constexpr bool can_resolve_b = requires { sizeof(decltype(container.resolve<B>())); };

    EXPECT_FALSE(can_resolve_a);
    EXPECT_FALSE(can_resolve_b);
}

TEST_F(integration_smoke_test_t, non_circular_dependency_works)
{
    struct A
    {
        A() {}
    };
    struct B
    {
        B(A const&) {}
    };

    auto container = root_container_t<>{};

    // This should be true - no circular dependency
    constexpr bool can_resolve_b = requires { container.resolve<B>(); };

    EXPECT_TRUE(can_resolve_b);
}

} // namespace
} // namespace dink
