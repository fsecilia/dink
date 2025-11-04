/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink::container {
namespace {

// =============================================================================
// CONTAINER HIERARCHIES - Parent/Child Relationships
// How child containers inherit and override parent bindings
// =============================================================================

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Delegation
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyDelegation : IntegrationTest {
  struct Type : Initialized {};
};

TEST_F(IntegrationTestHierarchyDelegation, child_finds_binding_in_parent) {
  auto parent = Container{bind<Type>()};
  auto child = Container{parent};

  auto result = child.template resolve<Type>();
  EXPECT_EQ(kInitialValue, result.value);
}

TEST_F(IntegrationTestHierarchyDelegation, child_overrides_parent_binding) {
  auto parent_factory = []() { return Product{kInitialValue}; };
  auto child_factory = []() { return Product{kModifiedValue}; };

  auto parent = Container{bind<Product>().via(parent_factory)};
  auto child = Container{parent, bind<Product>().via(child_factory)};

  auto parent_result = parent.template resolve<Product>();
  auto child_result = child.template resolve<Product>();

  EXPECT_EQ(kInitialValue, parent_result.value);
  EXPECT_EQ(kModifiedValue, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation, multi_level_hierarchy) {
  struct Grandparent {
    int_t value = 1;
    Grandparent() = default;
  };
  struct Parent {
    int_t value = 2;
    Parent() = default;
  };
  struct Child {
    int_t value = 3;
    Child() = default;
  };

  auto grandparent = Container{bind<Grandparent>()};
  auto parent = Container{grandparent, bind<Parent>()};
  auto child = Container{parent, bind<Child>()};

  // Child can resolve from all levels.
  auto grandparent_result = child.template resolve<Grandparent>();
  auto parent_result = child.template resolve<Parent>();
  auto child_result = child.template resolve<Child>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation,
       multi_level_hierarchy_via_factories) {
  auto grandparent_factory = []() { return Product{1}; };
  auto parent_factory = []() { return Product{2}; };
  auto child_factory = []() { return Product{3}; };

  auto grandparent = Container{bind<Product>().via(grandparent_factory)};
  auto parent = Container{grandparent, bind<Product>().via(parent_factory)};
  auto child = Container{parent, bind<Product>().via(child_factory)};

  auto grandparent_result = grandparent.template resolve<Product>();
  auto parent_result = parent.template resolve<Product>();
  auto child_result = child.template resolve<Product>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation,
       unbound_type_uses_fallback_in_hierarchy) {
  auto parent = Container{};
  auto child = Container{parent};

  // Uses fallback binding at the root level.
  auto result = child.template resolve<Type>();
  EXPECT_EQ(kInitialValue, result.value);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Singleton Sharing
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchySingletonSharing : IntegrationTest {};

TEST_F(IntegrationTestHierarchySingletonSharing,
       singleton_in_parent_shared_with_child) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       singleton_in_grandparent_shared_with_all) {
  struct Type : Singleton {};

  auto grandparent = Container{bind<Type>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       child_singleton_does_not_affect_parent) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  auto& child_ref = child.template resolve<Type&>();
  // Parent should create new instance (unbound type, promoted).
  auto& parent_ref = parent.template resolve<Type&>();

  EXPECT_NE(&child_ref, &parent_ref);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       parent_and_child_can_have_separate_singletons) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // Child overrides, so they should be different.
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Transient Behavior
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyTransient : IntegrationTest {
  struct Type : Initialized {};
};

TEST_F(IntegrationTestHierarchyTransient,
       transient_in_parent_creates_new_instances_for_child) {
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};

  auto parent_val1 = parent.template resolve<Type>();
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();

  EXPECT_EQ(0, parent_val1.id);
  EXPECT_EQ(1, child_val1.id);
  EXPECT_EQ(2, child_val2.id);
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyTransient,
       transient_in_grandparent_creates_new_instances_for_all) {
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto grandparent_val = grandparent.template resolve<Type>();
  auto parent_val = parent.template resolve<Type>();
  auto child_val = child.template resolve<Type>();

  EXPECT_EQ(0, grandparent_val.id);
  EXPECT_EQ(1, parent_val.id);
  EXPECT_EQ(2, child_val.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Promotion in Hierarchy
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyPromotion : IntegrationTest {};

TEST_F(IntegrationTestHierarchyPromotion,
       child_promotes_transient_from_parent) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};

  // Child requests by reference, should promote
  auto& child_ref1 = child.template resolve<Type&>();
  auto& child_ref2 = child.template resolve<Type&>();

  EXPECT_EQ(&child_ref1, &child_ref2);
  EXPECT_EQ(0, child_ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyPromotion,
       child_shares_parent_promoted_instance_when_delegating) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};  // Child has no binding, will delegate

  // Parent promotes to singleton when requested by reference.
  auto& parent_ref = parent.template resolve<Type&>();

  // Child delegates to parent, gets same promoted instance.
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);  // Same instance
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);  // Only one instance created
}

TEST_F(IntegrationTestHierarchyPromotion,
       child_has_separate_promoted_instance_with_own_binding) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Transient>()};

  // Each promotes separately because each has its own binding.
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_NE(&parent_ref, &child_ref);  // Different instances
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyPromotion,
       grandparent_parent_child_share_promoted_instance_when_delegating) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent};  // No binding, delegates to grandparent
  auto child =
      Container{parent};  // No binding, delegates to parent -> grandparent

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // All share grandparent's promoted instance
  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// Ancestry is part of a container's type, so ancestors can all have the same
// bindings but remain unique types and have separate cached instances.
TEST_F(IntegrationTestHierarchyPromotion,
       ancestry_with_same_bindings_promote_separate_instances) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent, bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Transient>()};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // Each has its own promoted instance.
  EXPECT_NE(&grandparent_ref, &parent_ref);
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, child_ref.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Value Resolution in Hierarchy
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyValueResolution : IntegrationTest {};

TEST_F(IntegrationTestHierarchyValueResolution,
       child_gets_copies_of_parent_singleton) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  // Child requests by value, gets copies of parent's singleton.
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();

  EXPECT_NE(&child_val1, &child_val2);   // Different copies
  EXPECT_EQ(0, child_val1.id);           // Both copies of same singleton (id 0)
  EXPECT_EQ(0, child_val2.id);           // Both copies of same singleton (id 0)
  EXPECT_EQ(1, Counted::num_instances);  // Only parent's singleton
}

TEST_F(IntegrationTestHierarchyValueResolution,
       parent_singleton_reference_differs_from_child_value_copies) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto child_val = child.template resolve<Type>();

  EXPECT_NE(&parent_ref, &child_val);    // Value is a copy
  EXPECT_EQ(0, parent_ref.id);           // Singleton
  EXPECT_EQ(0, child_val.id);            // Copy of same singleton
  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton
}

TEST_F(
    IntegrationTestHierarchyValueResolution,
    grandparent_singleton_reference_accessible_but_child_can_get_value_copies) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();
  auto child_val = child.template resolve<Type>();

  EXPECT_EQ(&grandparent_ref, &child_ref);  // References shared
  EXPECT_NE(&grandparent_ref, &child_val);  // Value is a copy
  EXPECT_EQ(0, grandparent_ref.id);         // Singleton
  EXPECT_EQ(0, child_val.id);               // Copy of same singleton
  EXPECT_EQ(1, Counted::num_instances);     // Only 1 singleton
}

}  // namespace
}  // namespace dink::container
