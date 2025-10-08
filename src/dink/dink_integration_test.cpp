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
struct instance_t
{
    int_t id = next_id++;
    instance_t(args_t...) noexcept {}
};

struct integration_smoke_test_t : Test
{
    using A = instance_t<>;
    using B = instance_t<A>;
    using C = instance_t<A, B>;
};

TEST_F(integration_smoke_test_t, default_ctor_transient)
{
    root_container_t<> container;

    auto a1 = container.resolve<A>();
    auto a2 = container.resolve<A>();

    ASSERT_NE(a1.id, a2.id); // Transient - different instances
}

TEST_F(integration_smoke_test_t, default_ctor_singleton)
{
    root_container_t<> container;

    auto& a1 = container.resolve<A&>();
    auto& a2 = container.resolve<A&>();

    ASSERT_EQ(&a1, &a2); // Singleton - same instance
}

TEST_F(integration_smoke_test_t, dependency_graph)
{
    root_container_t<> container;

    auto c = container.resolve<C>();

    EXPECT_TRUE(c.id > 0); // Successfully constructed with dependencies
}

TEST_F(integration_smoke_test_t, explicit_binding)
{
    using namespace dink;

    root_container_t container{bind<A>().to<A>().in<scopes::singleton_t>()};

    auto a1 = container.resolve<A>();
    auto a2 = container.resolve<A>();

    ASSERT_EQ(a1.id, a2.id); // Explicitly bound singleton
}

} // namespace
} // namespace dink
