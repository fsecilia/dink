/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/bindings.hpp>
#include <dink/container.hpp>

namespace dink {
namespace {

// Test instrumentation
inline static int_t next_index         = 0;
inline static int_t next_id            = 0;
inline static int_t total_dtors        = 0;
inline static int_t total_ctors        = 0;
inline static int_t total_copy_ctors   = 0;
inline static int_t total_move_ctors   = 0;
inline static int_t total_copy_assigns = 0;
inline static int_t total_move_assigns = 0;

void reset_test_counters() {
    next_index         = 0;
    next_id            = 0;
    total_dtors        = 0;
    total_ctors        = 0;
    total_copy_ctors   = 0;
    total_move_ctors   = 0;
    total_copy_assigns = 0;
    total_move_assigns = 0;
}

template <typename tag_t, typename... args_t>
struct constructed_from_t {
    int_t index = next_index++;
    int_t id    = next_id++;
    constructed_from_t(args_t...) noexcept { ++total_ctors; }
    ~constructed_from_t() { ++total_dtors; }
    constructed_from_t(constructed_from_t const& src) : id{src.id} { ++total_copy_ctors; }
    constructed_from_t(constructed_from_t&& src) : id{src.id} { ++total_move_ctors; }

    auto operator=(constructed_from_t const& src) -> constructed_from_t& {
        ++total_copy_ctors;
        id = src.id;
        return *this;
    }

    auto operator=(constructed_from_t&& src) -> constructed_from_t& {
        ++total_move_ctors;
        id = src.id;
        return *this;
    }
};

// Type aliases for common test types
struct no_deps_t : constructed_from_t<no_deps_t> {
    using constructed_from_t<no_deps_t>::constructed_from_t;
};
struct one_dep_t : constructed_from_t<one_dep_t, no_deps_t> {
    using constructed_from_t<one_dep_t, no_deps_t>::constructed_from_t;
};
struct two_deps_t : constructed_from_t<two_deps_t, no_deps_t, one_dep_t> {
    using constructed_from_t<two_deps_t, no_deps_t, one_dep_t>::constructed_from_t;
};
struct three_deps_t : constructed_from_t<three_deps_t, no_deps_t, one_dep_t, two_deps_t> {
    using constructed_from_t<three_deps_t, no_deps_t, one_dep_t, two_deps_t>::constructed_from_t;
};

//! factory that forwards directly to ctors; this adapts direct ctor calls to the generic discoverable factory api
// this isn't used atm, but it will be used to mirror a few tests
template <typename constructed_t>
class ctor_factory_t {
public:
    template <typename... args_t>
        requires std::constructible_from<constructed_t, args_t...>
    constexpr auto operator()(args_t&&... args) const -> constructed_t {
        return constructed_t{std::forward<args_t>(args)...};
    }
};
struct integration_smoke_test_t : Test {};

#if 1
TEST_F(integration_smoke_test_t, default_ctor_transient) {
    struct unique_tag_t {};
    using instance_t = constructed_from_t<unique_tag_t>;

    auto container = root_container_t<>{};
    auto a1        = container.resolve<instance_t>();
    auto a2        = container.resolve<instance_t>();

    ASSERT_NE(a1.id, a2.id);  // Transient - different instances
}

TEST_F(integration_smoke_test_t, default_ctor_singleton) {
    struct unique_tag_t {};
    using instance_t = constructed_from_t<unique_tag_t>;

    auto  container = root_container_t<>{};
    auto& a1        = container.resolve<instance_t&>();
    auto& a2        = container.resolve<instance_t&>();

    ASSERT_EQ(&a1, &a2);  // Singleton - same instance
}

TEST_F(integration_smoke_test_t, dependency_graph) {
    struct unique_tag_t {};
    using instance_t = constructed_from_t<unique_tag_t, constructed_from_t<unique_tag_t>,
                                          constructed_from_t<unique_tag_t, constructed_from_t<unique_tag_t>>>;

    auto container = root_container_t<>{};
    auto c         = container.resolve<instance_t>();

    EXPECT_TRUE(c.id > 0);  // Successfully constructed with dependencies
}

TEST_F(integration_smoke_test_t, explicit_binding) {
    struct unique_tag_t {};
    using instance_t = constructed_from_t<unique_tag_t>;

    auto container = root_container_t{bind<instance_t>().to<instance_t>().in<scope::singleton_t>()};
    auto a1        = container.resolve<instance_t>();
    auto a2        = container.resolve<instance_t>();

    ASSERT_EQ(a1.id, a2.id);  // Explicitly bound singleton
}
#endif

// Base test fixture
class ContainerTest : public Test {
protected:
    void SetUp() override { reset_test_counters(); }
};

// =============================================================================
// Basic Resolution Tests
// =============================================================================

TEST_F(ContainerTest, DefaultConstructionWithoutBinding) {
    auto container = root_container_t{};

    {
        struct local_type_t : no_deps_t {
            using no_deps_t::no_deps_t;
        };

        // 1 temporary constructed, then moved into named instance
        auto instance = container.resolve<local_type_t>();

        EXPECT_EQ(instance.id, 0);
        EXPECT_EQ(instance.index, 1);

        EXPECT_EQ(total_ctors, 1);
        EXPECT_EQ(total_dtors, 1);
        EXPECT_EQ(total_copy_ctors, 0);
        EXPECT_EQ(total_move_ctors, 1);
        EXPECT_EQ(total_copy_assigns, 0);
        EXPECT_EQ(total_move_assigns, 0);
    }

    EXPECT_EQ(total_ctors, 1);
    EXPECT_EQ(total_dtors, 2);
    EXPECT_EQ(total_copy_ctors, 0);
    EXPECT_EQ(total_move_ctors, 1);
    EXPECT_EQ(total_copy_assigns, 0);
    EXPECT_EQ(total_move_assigns, 0);
}

#if 1
TEST_F(ContainerTest, DefaultConstructionWithOneDependency) {
    struct unique_type_t : one_dep_t {};

    auto container = root_container_t{};
    auto instance  = container.resolve<unique_type_t>();

    EXPECT_EQ(instance.id, 1);
    EXPECT_EQ(total_ctors, 2);  // 1 dep + 1 main
}

TEST_F(ContainerTest, DefaultConstructionWithDependencies) {
    auto container = root_container_t{};
    auto instance  = container.resolve<three_deps_t>();

    EXPECT_EQ(instance.id, 7);
    EXPECT_EQ(total_ctors, 8);
}

TEST_F(ContainerTest, ExplicitBindingToImplementation) {
    struct interface_t {
        virtual ~interface_t() = default;

        interface_t()                   = default;
        interface_t(interface_t&&)      = default;
        interface_t(interface_t const&) = delete;
    };
    struct implementation_t : interface_t, no_deps_t {
        implementation_t()                        = default;
        implementation_t(implementation_t&&)      = default;
        implementation_t(implementation_t const&) = delete;
    };

    auto container = root_container_t{bind<interface_t>().to<implementation_t>().in<scope::singleton_t>()};

    auto& instance = container.resolve<interface_t&>();
    EXPECT_NE(dynamic_cast<implementation_t*>(&instance), nullptr);
}

// =============================================================================
// scope Behavior Tests
// =============================================================================

TEST_F(ContainerTest, TransientscopeCreatesNewInstances) {
    auto container = root_container_t{bind<no_deps_t>().to<no_deps_t>()};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(total_ctors, 2);
}

TEST_F(ContainerTest, SingletonscopeReusesSameInstance) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto a = container.resolve<unique_type_t>();
    auto b = container.resolve<unique_type_t>();

    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, DefaultscopeIsTransient) {
    auto container = root_container_t{};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(total_ctors, 2);
}

// =============================================================================
// Request Type Variation Tests
// =============================================================================

TEST_F(ContainerTest, ReferenceRequestForcesSingleton) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::transient_t>()};

    auto& a = container.resolve<unique_type_t&>();
    auto& b = container.resolve<unique_type_t&>();

    EXPECT_EQ(&a, &b);
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, RValueReferenceRequestForcesTransient) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};
    auto a         = container.resolve<unique_type_t&&>();
    auto b         = container.resolve<unique_type_t&&>();

    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(total_ctors, 1);  // 1 constructed in cache, 2 copies made
}

TEST_F(ContainerTest, PointerRequestForcesSingleton) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{};

    auto* a = container.resolve<unique_type_t*>();
    auto* b = container.resolve<unique_type_t*>();

    EXPECT_EQ(a, b);
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, SharedPtrRequest) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto a = container.resolve<std::shared_ptr<unique_type_t>>();
    auto b = container.resolve<std::shared_ptr<unique_type_t>>();

    EXPECT_EQ(a->id, b->id);
    EXPECT_EQ(a.get(), b.get());
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, UniquePtrRequestForcesTransient) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto a = container.resolve<std::unique_ptr<unique_type_t>>();
    auto b = container.resolve<std::unique_ptr<unique_type_t>>();

    EXPECT_NE(a.get(), b.get());
    EXPECT_EQ(a->id, b->id);
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, WeakPtrRequest) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto shared = container.resolve<std::shared_ptr<unique_type_t>>();
    auto weak   = container.resolve<std::weak_ptr<unique_type_t>>();

    auto locked = weak.lock();
    ASSERT_NE(locked, nullptr);
    EXPECT_EQ(locked->id, shared->id);
    EXPECT_EQ(total_ctors, 1);
}

// =============================================================================
// Factory Tests
// =============================================================================

TEST_F(ContainerTest, CustomFactory) {
    struct unique_type_t : no_deps_t {};

    int_t factory_calls = 0;
    auto  container     = root_container_t{bind<unique_type_t>()
                                          .template to_factory<unique_type_t>([&factory_calls]() {
                                              ++factory_calls;
                                              return unique_type_t{};
                                          })
                                          .in<scope::transient_t>()};

    auto a = container.resolve<unique_type_t>();
    auto b = container.resolve<unique_type_t>();

    EXPECT_EQ(factory_calls, 2);
    EXPECT_NE(a.id, b.id);
}

TEST_F(ContainerTest, FactoryWithDependencies) {
    struct local_type_t : two_deps_t {
        using two_deps_t::two_deps_t;
    };

    auto container = root_container_t{bind<local_type_t>().template to_factory<local_type_t>(
        [](no_deps_t a, one_dep_t b) { return local_type_t{a, b}; })};

    auto instance = container.resolve<local_type_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_ctors, 4);  // arg no_deps(1) + arg one_dep(2) + final two_deps(1)
}

// =============================================================================
// Reference and Prototype Tests
// =============================================================================

TEST_F(ContainerTest, InternalReferenceBinding) {
    no_deps_t instance;
    auto      original_id = instance.id;

    auto container = root_container_t{bind<no_deps_t>().to_internal_reference(std::move(instance))};

    auto& a = container.resolve<no_deps_t&>();
    auto& b = container.resolve<no_deps_t&>();

    EXPECT_EQ(&a, &b);
    EXPECT_EQ(a.id, original_id);
}

TEST_F(ContainerTest, ExternalReferenceBinding) {
    no_deps_t instance;
    auto      original_id = instance.id;

    auto container = root_container_t{bind<no_deps_t>().to_external_reference(instance)};

    auto& resolved = container.resolve<no_deps_t&>();

    EXPECT_EQ(&resolved, &instance);
    EXPECT_EQ(resolved.id, original_id);
}

TEST_F(ContainerTest, InternalPrototypeBinding) {
    no_deps_t prototype;

    auto container = root_container_t{bind<no_deps_t>().to_internal_prototype(std::move(prototype))};

    auto a = container.resolve<no_deps_t>();
    auto b = container.resolve<no_deps_t>();

    EXPECT_NE(&a, &b);      // Different instances
    EXPECT_EQ(a.id, b.id);  // Same value
}

TEST_F(ContainerTest, ExternalPrototypeBinding) {
    no_deps_t prototype;

    auto container = root_container_t{bind<no_deps_t>().to_external_prototype(prototype)};

    auto const expected_a_id = 3;
    prototype.id             = expected_a_id;
    auto a                   = container.resolve<no_deps_t>();

    auto const expected_b_id = 4;
    prototype.id             = expected_b_id;
    auto b                   = container.resolve<no_deps_t>();

    EXPECT_NE(&a, &b);  // Different instances

    EXPECT_EQ(expected_a_id, a.id);
    EXPECT_EQ(expected_b_id, b.id);
}

// =============================================================================
// Container Hierarchy Tests
// =============================================================================

TEST_F(ContainerTest, NestedResolvesFromRoot) {
    struct unique_type_t : no_deps_t {};

    auto root   = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};
    auto nested = nested_container_t{root};

    auto& from_root   = root.resolve<unique_type_t&>();
    auto& from_nested = nested.resolve<unique_type_t&>();

    EXPECT_EQ(&from_root, &from_nested);
    EXPECT_EQ(total_ctors, 1);
}

TEST_F(ContainerTest, NestedOverridesrootBinding) {
    struct unique_type_t : no_deps_t {};

    auto root   = container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};
    auto nested = container_t{root, bind<unique_type_t>().to<unique_type_t>().in<scope::transient_t>()};

    auto& from_root    = root.resolve<unique_type_t&>();
    auto  from_nested1 = nested.resolve<unique_type_t>();
    auto  from_nested2 = nested.resolve<unique_type_t>();

    EXPECT_NE(from_root.id, from_nested1.id);
    EXPECT_NE(from_nested1.id, from_nested2.id);
    EXPECT_EQ(total_ctors, 3);
}

TEST_F(ContainerTest, NestedContainerSingletonScoping) {
    struct unique_type_t : no_deps_t {};

    auto root    = root_container_t{};
    auto nested1 = nested_container_t{root, bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};
    auto nested2 = nested_container_t{root, bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto& from_nested1_a = nested1.resolve<unique_type_t&>();
    auto& from_nested1_b = nested1.resolve<unique_type_t&>();
    auto& from_nested2   = nested2.resolve<unique_type_t&>();

    EXPECT_EQ(&from_nested1_a, &from_nested1_b);  // Same within nested1
    EXPECT_NE(&from_nested1_a, &from_nested2);    // Different across nestedren
    EXPECT_EQ(total_ctors, 2);
}

// =============================================================================
// Complex Dependency Graph Tests
// =============================================================================

TEST_F(ContainerTest, DeepDependencyChain) {
    // A -> B -> C -> D
    struct d_t : constructed_from_t<d_t> {};

    struct c_t : constructed_from_t<d_t, d_t> {};

    struct b_t : constructed_from_t<b_t, c_t> {};

    struct a_t : constructed_from_t<a_t, b_t> {};

    auto container = root_container_t{};
    auto instance  = container.resolve<a_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_ctors, 4);
}

TEST_F(ContainerTest, DiamondDependency) {
    // A -> B -> D
    // A -> C -> D
    // D should be singleton to avoid double construction
    struct d_t : constructed_from_t<d_t> {};

    struct b_t : constructed_from_t<b_t, d_t&> {};

    struct c_t : constructed_from_t<c_t, d_t&> {};

    struct a_t : constructed_from_t<a_t, b_t, c_t> {};

    auto container = root_container_t{};
    auto instance  = container.resolve<a_t>();

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(total_ctors, 4);  // D (once), B, C, A
}

TEST_F(ContainerTest, MixedScopesInDependencyChain) {
    struct dep_t : constructed_from_t<dep_t> {};
    struct service_t : constructed_from_t<service_t> {};

    auto container = root_container_t{bind<dep_t>().to<dep_t>().in<scope::singleton_t>(),
                                      bind<service_t>().to<service_t>().in<scope::transient_t>()};

    auto a = container.resolve<service_t>();
    auto b = container.resolve<service_t>();

    EXPECT_NE(a.id, b.id);              // Service is transient
    EXPECT_EQ(total_ctors, 2);          // 1 singleton dep, 2 transient services
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(ContainerTest, MultipleBindingsFirstWins) {
    int_t first_factory_calls  = 0;
    int_t second_factory_calls = 0;

    auto container = root_container_t{bind<no_deps_t>().to_factory<no_deps_t>([&first_factory_calls]() {
                                          ++first_factory_calls;
                                          return no_deps_t{};
                                      }),
                                      bind<no_deps_t>().to_factory<no_deps_t>([&second_factory_calls]() {
                                          ++second_factory_calls;
                                          return no_deps_t{};
                                      })};

    container.resolve<no_deps_t>();

    EXPECT_EQ(first_factory_calls, 1);
    EXPECT_EQ(second_factory_calls, 0);
}

TEST_F(ContainerTest, MoveOnlyType) {
    struct move_only_t : constructed_from_t<move_only_t> {
        move_only_t()                              = default;
        move_only_t(move_only_t const&)            = delete;
        move_only_t& operator=(move_only_t const&) = delete;
        move_only_t(move_only_t&&)                 = default;
        move_only_t& operator=(move_only_t&&)      = default;
    };

    auto container = root_container_t{};

    auto instance = container.resolve<move_only_t>();
    EXPECT_EQ(instance.id, 0);
    EXPECT_EQ(instance.index, 1);

    auto ptr = container.resolve<std::unique_ptr<move_only_t>>();
    EXPECT_EQ(ptr->id, 1);
    EXPECT_EQ(ptr->index, 2);
}

TEST_F(ContainerTest, ConstRequestTypes) {
    struct unique_type_t : no_deps_t {};

    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    auto const& const_ref = container.resolve<unique_type_t const&>();
    auto const* const_ptr = container.resolve<unique_type_t const*>();

    EXPECT_EQ(const_ptr, &const_ref);
    EXPECT_EQ(total_ctors, 1);
}

// =============================================================================
// Performance/Stress Tests
// =============================================================================

TEST_F(ContainerTest, LargeDependencyGraphPerformance) {
    struct level5_t : constructed_from_t<level5_t> {};
    struct level4_t : constructed_from_t<level4_t, level5_t, level5_t, level5_t> {};
    struct level3_t : constructed_from_t<level3_t, level4_t, level4_t> {};
    struct level2_t : constructed_from_t<level2_t, level3_t, level3_t> {};
    struct level1_t : constructed_from_t<level1_t, level2_t> {};

    auto container = root_container_t{bind<level5_t>().to<level5_t>().in<scope::singleton_t>()};

    auto start_constructions = total_ctors;
    auto instance            = container.resolve<level1_t>();
    auto constructions_made  = total_ctors - start_constructions;

    EXPECT_GT(instance.id, 0);
    EXPECT_EQ(constructions_made, 9);  // 5 4 4 3 4 4 3 2 1
}

TEST_F(ContainerTest, ThreadSafetyOfRootSingletons) {
    struct unique_type_t : no_deps_t {};

    // Root container singletons use static locals which are thread-safe in C++11
    auto container = root_container_t{bind<unique_type_t>().to<unique_type_t>().in<scope::singleton_t>()};

    unique_type_t* ptr1 = nullptr;
    unique_type_t* ptr2 = nullptr;

    auto thread1 = std::thread([&]() { ptr1 = &container.resolve<unique_type_t&>(); });
    auto thread2 = std::thread([&]() { ptr2 = &container.resolve<unique_type_t&>(); });

    thread1.join();
    thread2.join();

    EXPECT_EQ(ptr1, ptr2);
    EXPECT_EQ(total_ctors, 1);
}
#endif

}  // namespace
}  // namespace dink
