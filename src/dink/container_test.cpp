/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/bindings.hpp>
#include <dink/container.hpp>
#include <dink/lifecycle.hpp>
#include <memory>

namespace dink {
// namespace {

// Test instrumentation
inline static int_t next_id = 1;
inline static int_t total_constructions = 0;
inline static int_t total_destructions = 0;

void reset_test_counters()
{
    next_id = 1;
    total_constructions = 0;
    total_destructions = 0;
}

template <typename... args_t>
struct constructed_from_t
{
    int_t id = next_id++;
    constructed_from_t(args_t...) noexcept { ++total_constructions; }
    ~constructed_from_t() { ++total_destructions; }

    constructed_from_t(constructed_from_t const&) = default;
};

// Type aliases for common test types
using no_deps_t = constructed_from_t<>;
using one_dep_t = constructed_from_t<no_deps_t>;
using two_deps_t = constructed_from_t<no_deps_t, one_dep_t>;
using three_deps_t = constructed_from_t<no_deps_t, one_dep_t, two_deps_t>;

// Base test fixture
class ContainerTest : public Test
{
protected:
    void SetUp() override { reset_test_counters(); }
};

// =============================================================================
// Basic Resolution Tests
// =============================================================================

#if 0
TEST_F(ContainerTest, DefaultConstructionWithoutBinding)
{
    auto container = root_container_t{};
    auto instance = container.resolve<no_deps_t>();

    EXPECT_EQ(instance.id, 1);
    EXPECT_EQ(total_constructions, 1);
}

TEST_F(ContainerTest, DefaultConstructionWithOneDependency)
{
    auto container = root_container_t{};
    auto instance = container.resolve<one_dep_t>();

    EXPECT_EQ(instance.id, 2);
    EXPECT_EQ(total_constructions, 2); // 1 dep + 1 main
}

TEST_F(ContainerTest, DefaultConstructionWithDependencies)
{
    auto container = root_container_t{};
    auto instance = container.resolve<three_deps_t>();

    EXPECT_EQ(instance.id, 8);
    EXPECT_EQ(total_constructions, 8);
}

TEST_F(ContainerTest, ExplicitBindingToImplementation)
{
    struct interface_t
    {
        virtual ~interface_t() = default;

        interface_t() = default;
        interface_t(interface_t&&) = default;
        interface_t(interface_t const&) = delete;
    };
    struct implementation_t : interface_t, constructed_from_t<>
    {
        implementation_t() = default;
        implementation_t(implementation_t&&) = default;
        implementation_t(implementation_t const&) = delete;
    };

    auto container = root_container_t{bind<interface_t>().to<implementation_t>().in<scopes::singleton_t>()};

    auto& instance = container.resolve<interface_t&>();
    EXPECT_NE(dynamic_cast<implementation_t*>(&instance), nullptr);
}

// =============================================================================
// Scope Behavior Tests
// =============================================================================

TEST_F(ContainerTest, TransientScopeCreatesNewInstances)
{
    auto container = root_container_t{bind<no_deps_t>().to<no_deps_t>()};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(total_constructions, 2);
}

TEST_F(ContainerTest, SingletonScopeReusesSameInstance)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto a = container.resolve<unique_type_t>();
    auto b = container.resolve<unique_type_t>();

    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(total_constructions, 1);
}

TEST_F(ContainerTest, DefaultScopeIsTransient)
{
    auto container = root_container_t{};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(total_constructions, 2);
}

// =============================================================================
// Request Type Variation Tests
// =============================================================================

TEST_F(ContainerTest, ReferenceRequestForcesSingleton)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::transient_t>()};

    auto& a = container.resolve<unique_type_t&>();
    auto& b = container.resolve<unique_type_t&>();

    EXPECT_EQ(&a, &b);
    EXPECT_EQ(total_constructions, 1);
}

TEST_F(ContainerTest, RValueReferenceRequestForcesTransient)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};
    auto a = container.resolve<unique_type_t&&>();
    auto b = container.resolve<unique_type_t&&>();

    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(total_constructions, 2);
}

TEST_F(ContainerTest, PointerRequestForcesSingleton)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{};

    auto* a = container.resolve<unique_type_t*>();
    auto* b = container.resolve<unique_type_t*>();

    EXPECT_EQ(a, b);
    EXPECT_EQ(total_constructions, 1);
}

TEST_F(ContainerTest, SharedPtrRequest)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto a = container.resolve<std::shared_ptr<unique_type_t>>();
    auto b = container.resolve<std::shared_ptr<unique_type_t>>();

    EXPECT_EQ(a->id, b->id);
    EXPECT_EQ(a.get(), b.get());
    EXPECT_EQ(total_constructions, 1);
}

TEST_F(ContainerTest, UniquePtrRequestForcesTransient)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto a = container.resolve<std::unique_ptr<unique_type_t>>();
    auto b = container.resolve<std::unique_ptr<unique_type_t>>();

    EXPECT_NE(a->id, b->id);
    EXPECT_EQ(total_constructions, 2);
}

TEST_F(ContainerTest, WeakPtrRequest)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto shared = container.resolve<std::shared_ptr<unique_type_t>>();
    auto weak = container.resolve<std::weak_ptr<unique_type_t>>();

    auto locked = weak.lock();
    ASSERT_NE(locked, nullptr);
    EXPECT_EQ(locked->id, shared->id);
    EXPECT_EQ(total_constructions, 1);
}

// =============================================================================
// Factory Tests
// =============================================================================

TEST_F(ContainerTest, CustomFactory)
{
    struct unique_type_t : no_deps_t
    {};

    int_t factory_calls = 0;
    auto container = root_container_t{
        bind<unique_type_t>()
            .template to_factory<unique_type_t>([&factory_calls]() {
                ++factory_calls;
                return unique_type_t{};
            })
            .in<scopes::transient_t>()
    };

    auto a = container.resolve<unique_type_t>();
    auto b = container.resolve<unique_type_t>();

    EXPECT_EQ(factory_calls, 2);
    EXPECT_NE(a.id, b.id);
}

TEST_F(ContainerTest, FactoryWithDependencies)
{
    struct unique_type_t : two_deps_t
    {
        using two_deps_t::two_deps_t;
    };

    auto container = root_container_t{bind<unique_type_t>().template to_factory<unique_type_t>(
        [](no_deps_t a, one_dep_t b) { return unique_type_t{a, b}; }
    )};

    auto instance = container.resolve<unique_type_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_constructions, 4); // arg no_deps(1) + arg one_dep(2) + final two_deps(1)
}

// =============================================================================
// Reference and Prototype Tests
// =============================================================================

TEST_F(ContainerTest, InternalReferenceBinding)
{
    no_deps_t instance;
    auto original_id = instance.id;

    auto container = root_container_t{bind<no_deps_t>().to_internal_reference(std::move(instance))};

    auto& a = container.resolve<no_deps_t&>();
    auto& b = container.resolve<no_deps_t&>();

    EXPECT_EQ(&a, &b);
    EXPECT_EQ(a.id, original_id);
}

TEST_F(ContainerTest, ExternalReferenceBinding)
{
    no_deps_t instance;
    auto original_id = instance.id;

    auto container = root_container_t{bind<no_deps_t>().to_external_reference(instance)};

    auto& resolved = container.resolve<no_deps_t&>();

    EXPECT_EQ(&resolved, &instance);
    EXPECT_EQ(resolved.id, original_id);
}

TEST_F(ContainerTest, InternalPrototypeBinding)
{
    no_deps_t prototype;

    auto container = root_container_t{bind<no_deps_t>().to_internal_prototype(std::move(prototype))};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(&a, &b); // Different instances
    EXPECT_EQ(a.id, b.id); // Same value
}

TEST_F(ContainerTest, ExternalPrototypeBinding)
{
    no_deps_t prototype;

    auto container = root_container_t{bind<no_deps_t>().to_external_prototype(prototype)};

    auto const expected_a_id = 3;
    prototype.id = expected_a_id;
    auto a = container.resolve<no_deps_t>();

    auto const expected_b_id = 4;
    prototype.id = expected_b_id;
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(&a, &b); // Different instances

    EXPECT_EQ(expected_a_id, a.id);
    EXPECT_EQ(expected_b_id, b.id);
}

// =============================================================================
// Container Hierarchy Tests
// =============================================================================

TEST_F(ContainerTest, ChildResolvesFromParent)
{
    struct unique_type_t : no_deps_t
    {};

    auto parent = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto child = child_container_t<decltype(parent)>{parent};

    auto& from_parent = parent.resolve<unique_type_t&>();
    auto& from_child = child.resolve<unique_type_t&>();

    EXPECT_EQ(&from_parent, &from_child);
    EXPECT_EQ(total_constructions, 1);
}
#endif
TEST_F(ContainerTest, ChildOverridesParentBinding)
{
    struct unique_type_t : no_deps_t
    {};

    auto parent = container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};
    auto child = container_t{parent, bind<unique_type_t>().to<unique_type_t>().in<scopes::transient_t>()};

    auto& from_parent = parent.resolve<unique_type_t&>();
    auto from_child1 = child.resolve<unique_type_t>();
    auto from_child2 = child.resolve<unique_type_t>();

    EXPECT_NE(from_parent.id, from_child1.id);
    EXPECT_NE(from_child1.id, from_child2.id);
    EXPECT_EQ(total_constructions, 3);
}
#if 0
TEST_F(ContainerTest, NestedContainerSingletonScoping)
{
    struct unique_type_t : no_deps_t
    {};

    auto parent = root_container_t{};
    auto child1 = child_container_t{parent, bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};
    auto child2 = child_container_t{parent, bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto& from_child1_a = child1.resolve<unique_type_t&>();
    auto& from_child1_b = child1.resolve<unique_type_t&>();
    auto& from_child2 = child2.resolve<unique_type_t&>();

    EXPECT_EQ(&from_child1_a, &from_child1_b); // Same within child1
    EXPECT_NE(&from_child1_a, &from_child2); // Different across children
    EXPECT_EQ(total_constructions, 2);
}

// =============================================================================
// Complex Dependency Graph Tests
// =============================================================================

TEST_F(ContainerTest, DeepDependencyChain)
{
    // A -> B -> C -> D
    struct d_t : constructed_from_t<>
    {};

    struct c_t : constructed_from_t<d_t>
    {};

    struct b_t : constructed_from_t<c_t>
    {};

    struct a_t : constructed_from_t<b_t>
    {};

    auto container = root_container_t{};
    auto instance = container.resolve<a_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_constructions, 4);
}

TEST_F(ContainerTest, DiamondDependency)
{
    // A -> B -> D
    // A -> C -> D
    // D should be singleton to avoid double construction
    struct d_t : constructed_from_t<>
    {};

    struct b_t : constructed_from_t<d_t&>
    {};

    struct c_t : constructed_from_t<d_t&>
    {};

    struct a_t : constructed_from_t<b_t, c_t>
    {};

    auto container = root_container_t{};
    auto instance = container.resolve<a_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_constructions, 4); // D (once), B, C, A
}

TEST_F(ContainerTest, MixedScopesInDependencyChain)
{
    struct dep_t : constructed_from_t<>
    {};

    using service_t = constructed_from_t<dep_t>;

    auto container = root_container_t{
        bind<dep_t>().to<dep_t>().in<scopes::singleton_t>(), bind<service_t>().to<service_t>().in<scopes::transient_t>()
    };

    auto a = container.resolve<service_t>();
    auto b = container.resolve<service_t>();

    EXPECT_NE(a.id, b.id); // Service is transient
    EXPECT_EQ(total_constructions, 3); // 1 singleton dep, 2 transient services
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(ContainerTest, MultipleBindingsFirstWins)
{
    int_t first_factory_calls = 0;
    int_t second_factory_calls = 0;

    auto container = root_container_t{
        bind<no_deps_t>().to_factory<no_deps_t>([&first_factory_calls]() {
            ++first_factory_calls;
            return no_deps_t{};
        }),
        bind<no_deps_t>().to_factory<no_deps_t>([&second_factory_calls]() {
            ++second_factory_calls;
            return no_deps_t{};
        })
    };

    container.resolve<no_deps_t>();

    EXPECT_EQ(first_factory_calls, 1);
    EXPECT_EQ(second_factory_calls, 0);
}

TEST_F(ContainerTest, MoveOnlyType)
{
    struct move_only_t : constructed_from_t<>
    {
        move_only_t() = default;
        move_only_t(move_only_t const&) = delete;
        move_only_t& operator=(move_only_t const&) = delete;
        move_only_t(move_only_t&&) = default;
        move_only_t& operator=(move_only_t&&) = default;
    };

    auto container = root_container_t{};

    auto instance = container.resolve<move_only_t>();
    EXPECT_GT(instance.id, 0);

    auto ptr = container.resolve<std::unique_ptr<move_only_t>>();
    EXPECT_GT(ptr->id, 0);
}

TEST_F(ContainerTest, ConstRequestTypes)
{
    struct unique_type_t : no_deps_t
    {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    auto const& const_ref = container.resolve<unique_type_t const&>();
    auto const* const_ptr = container.resolve<unique_type_t const*>();

    EXPECT_EQ(const_ptr, &const_ref);
    EXPECT_EQ(total_constructions, 1);
}

// =============================================================================
// Circular Dependency Tests (should fail to compile if uncommented)
// =============================================================================

/*
TEST_F(ContainerTest, CircularDependencyDetection)
{
    // This should fail to compile with "circular dependency detected"
    struct a_t;
    struct b_t : constructed_from_t<a_t> {};
    struct a_t : constructed_from_t<b_t> {};
    
    auto container = root_container_t{};
    // container.resolve<a_t>(); // Should not compile
}
*/

// =============================================================================
// Performance/Stress Tests
// =============================================================================

TEST_F(ContainerTest, LargeDependencyGraphPerformance)
{
    struct level5_t : constructed_from_t<>
    {};

    struct level4_t : constructed_from_t<level5_t, level5_t, level5_t>
    {};

    struct level3_t : constructed_from_t<level4_t, level4_t>
    {};

    struct level2_t : constructed_from_t<level3_t, level3_t>
    {};

    struct level1_t : constructed_from_t<level2_t>
    {};

    auto container = root_container_t{bind<level5_t>().to<level5_t>().in<scopes::singleton_t>()};

    auto start_constructions = total_constructions;
    auto instance = container.resolve<level1_t>();
    auto constructions_made = total_constructions - start_constructions;

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(constructions_made, 9); // 5 4 4 3 4 4 3 2 1
}

TEST_F(ContainerTest, ThreadSafetyOfRootSingletons)
{
    struct unique_type_t : no_deps_t
    {};

    // Root container singletons use static locals which are thread-safe in C++11
    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scopes::singleton_t>()};

    unique_type_t* ptr1 = nullptr;
    unique_type_t* ptr2 = nullptr;

    auto thread1 = std::thread([&]() { ptr1 = &container.resolve<unique_type_t&>(); });
    auto thread2 = std::thread([&]() { ptr2 = &container.resolve<unique_type_t&>(); });

    thread1.join();
    thread2.join();

    EXPECT_EQ(ptr1, ptr2);
    EXPECT_EQ(total_constructions, 1);
}
#endif

// } // namespace
} // namespace dink
