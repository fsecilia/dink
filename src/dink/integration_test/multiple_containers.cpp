/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink::container {
namespace {

// =============================================================================
// CACHE STRATEGIES - Per-Type vs Per-Instance
// Choosing between Meyers singleton and instance caches
// =============================================================================

// ----------------------------------------------------------------------------
// Per-Type Cache Tests
// ----------------------------------------------------------------------------

struct IntegrationTestPerTypeCache : IntegrationTest {};

TEST_F(IntegrationTestPerTypeCache,
       containers_with_same_type_share_bound_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 = Container{parent, bind<Type>().in<scope::Singleton>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Singleton>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       containers_with_same_type_share_promoted_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 = Container{parent, bind<Type>().in<scope::Transient>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Transient>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type share per-type cache, even when promoted.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       containers_with_different_types_do_not_share_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have different bindings, so they have different types.
  auto child1 = Container{parent, bind<Type>().in<scope::Singleton>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Transient>()};
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Children with different types do not share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       dink_unique_container_creates_distinct_types) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};

  // These containers have unique types.
  auto child1 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  auto child2 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Children with unique types do not share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       repeated_macro_invocations_create_unique_types) {
  auto c1 = dink_unique_container();
  auto c2 = dink_unique_container(c1);
  auto c3 = dink_unique_container(c1);

  static_assert(!std::same_as<decltype(c1), decltype(c2)>);
  static_assert(!std::same_as<decltype(c2), decltype(c3)>);
  static_assert(!std::same_as<decltype(c1), decltype(c3)>);
}

// ----------------------------------------------------------------------------
// Per-Instance Cache Tests
// ----------------------------------------------------------------------------

struct IntegrationTestPerInstanceCache : IntegrationTest {};

TEST_F(IntegrationTestPerInstanceCache,
       containers_with_same_type_do_not_share_share_bound_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Singleton>()};
  auto child2 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Singleton>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type do not share per-instance cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerInstanceCache,
       containers_with_same_type_do_not_share_promoted_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Transient>()};
  auto child2 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Transient>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type do not share per-instance cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// =============================================================================
// COMPLEX SCENARIOS
// Multiple features working together
// =============================================================================

// ----------------------------------------------------------------------------
// Complex Hierarchical Scenarios
// ----------------------------------------------------------------------------

struct IntegrationTestComplexScenarios : IntegrationTest {};

TEST_F(IntegrationTestComplexScenarios, mixed_scopes_across_hierarchy) {
  struct SingletonInGrandparent : Singleton {};
  struct TransientInParent : Initialized {};
  struct SingletonInChild : Singleton {};

  auto grandparent =
      Container{bind<SingletonInGrandparent>().in<scope::Singleton>()};
  auto parent =
      Container{grandparent, bind<TransientInParent>().in<scope::Transient>()};
  auto child =
      Container{parent, bind<SingletonInChild>().in<scope::Singleton>()};

  // Singleton from grandparent shared.
  auto& sg1 = child.template resolve<SingletonInGrandparent&>();
  auto& sg2 = child.template resolve<SingletonInGrandparent&>();
  EXPECT_EQ(&sg1, &sg2);
  EXPECT_EQ(0, sg1.id);

  // Transient from parent creates new instances.
  auto tp1 = child.template resolve<TransientInParent>();
  auto tp2 = child.template resolve<TransientInParent>();
  EXPECT_NE(tp1.id, tp2.id);
  EXPECT_EQ(1, tp1.id);
  EXPECT_EQ(2, tp2.id);

  // Singleton in child
  auto& sc1 = child.template resolve<SingletonInChild&>();
  auto& sc2 = child.template resolve<SingletonInChild&>();
  EXPECT_EQ(&sc1, &sc2);
  EXPECT_EQ(3, sc1.id);

  EXPECT_EQ(4, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios, dependency_chain_across_hierarchy) {
  struct GrandparentDep : Singleton {};
  struct ParentDep : Initialized {
    GrandparentDep* grandparent_dep;
    explicit ParentDep(GrandparentDep& d) : grandparent_dep{&d} {}
  };
  struct ChildService : Singleton {
    ParentDep* parent_dep;
    explicit ChildService(ParentDep& d) : parent_dep{&d} {}
  };

  auto grandparent = Container{bind<GrandparentDep>().in<scope::Singleton>()};
  auto parent = Container{grandparent};  // Unbound, will be promoted.
  auto child = Container{parent};        // Unbound, will be promoted.

  auto& service = child.template resolve<ChildService&>();

  EXPECT_EQ(0, service.parent_dep->grandparent_dep->id);  // singleton
  EXPECT_EQ(1, service.parent_dep->id);                   // promoted in parent
  EXPECT_EQ(2, service.id);                               // promoted in child
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       promotion_and_value_resolution_across_hierarchy) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  // Parent transient promoted to singleton.
  auto& parent_ref1 = parent.template resolve<Type&>();
  auto& parent_ref2 = parent.template resolve<Type&>();
  EXPECT_EQ(&parent_ref1, &parent_ref2);
  EXPECT_EQ(0, parent_ref1.id);

  // Child singleton.
  auto& child_ref = child.template resolve<Type&>();
  EXPECT_EQ(1, child_ref.id);

  // Child singleton values are copies.
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();
  EXPECT_NE(&child_val1, &child_val2);  // Different copies
  EXPECT_EQ(1, child_val1.id);          // copy of child singleton
  EXPECT_EQ(1, child_val2.id);          // copy of child singleton

  EXPECT_EQ(2, Counted::num_instances);  // 1 parent (promoted) + 1 child
}

TEST_F(IntegrationTestComplexScenarios,
       promoted_unbound_instances_are_root_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       delegated_transient_promotions_are_shared) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child1 = Container{parent};
  auto child2 = Container{parent};

  // Both children delegate to parent, share parent's promoted transient.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance.
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       delegated_unbound_promotions_are_shared) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child1 = Container{parent};
  auto child2 = Container{parent};

  // Both children delegate to parent and share parent's promoted, unbound
  // instance.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance.
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       deep_hierarchy_with_multiple_overrides) {
  auto level0_factory = []() { return Product{0}; };
  auto level2_factory = []() { return Product{2}; };
  auto level4_factory = []() { return Product{4}; };

  auto level0 = Container{bind<Product>().via(level0_factory)};
  auto level1 = Container{level0};
  auto level2 = Container{level1, bind<Product>().via(level2_factory)};
  auto level3 = Container{level2};
  auto level4 = Container{level3, bind<Product>().via(level4_factory)};

  auto r0 = level0.template resolve<Product>();
  auto r1 = level1.template resolve<Product>();
  auto r2 = level2.template resolve<Product>();
  auto r3 = level3.template resolve<Product>();
  auto r4 = level4.template resolve<Product>();

  EXPECT_EQ(0, r0.value);
  EXPECT_EQ(0, r1.value);  // Inherits from level0
  EXPECT_EQ(2, r2.value);  // Overrides
  EXPECT_EQ(2, r3.value);  // Inherits from level2
  EXPECT_EQ(4, r4.value);  // Overrides
}

}  // namespace
}  // namespace dink::container
