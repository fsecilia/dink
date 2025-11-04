// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/binding.hpp>

namespace dink {
namespace {

//! Tests ctors and deduction guides.
struct ContainerCtorTest {
  using Binding0 = Binding<int_t, scope::Transient, provider::Ctor<int_t>>;
  using Binding1 = Binding<uint_t, scope::Transient, provider::Ctor<uint_t>>;
  using Binding2 = Binding<char, scope::Transient, provider::Ctor<char>>;

  static auto constexpr binding0 = Binding0{};
  static auto constexpr binding1 = Binding1{};
  static auto constexpr binding2 = Binding2{};

  // These have to use live types to match the deduction guides.
  using Cache = cache::Type;
  using Dispatcher = Dispatcher<>;

  using Parent = Container<Config<>, Cache, Dispatcher, void>;
  Parent parent;

  struct Tag {};

  // Ctors producing empty config.

  static_assert(std::same_as<Container<Config<>, Cache, Dispatcher, void>,
                             decltype(Container{})>,
                "empty args should produce empty Config");

  static_assert(std::same_as<Container<Config<>, Cache, Dispatcher, void, Tag>,
                             decltype(Container{Tag{}})>,
                "tag should produce empty Config");

  static_assert(
      std::same_as<Container<Config<>, Cache, Dispatcher, Parent, void>,
                   decltype(Container{parent})>,
      "parent should produce empty Config");

  static_assert(
      std::same_as<Container<Config<>, Cache, Dispatcher, Parent, void>,
                   decltype(Container{parent, Cache{}})>,
      "parent and cache should produce empty Config");

  static_assert(
      std::same_as<Container<Config<>, Cache, Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent})>,
      "tag and parent should produce empty Config");

  static_assert(
      std::same_as<Container<Config<>, Cache, Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent, Cache{}})>,
      "tag, parent, and cache should produce empty Config");

  // Single-element ctors.

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, void>,
                   decltype(Container{binding0})>,
      "single arg should produce single-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, void, Tag>,
                   decltype(Container{Tag{}, binding0})>,
      "tag and single arg should produce single-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, Parent, void>,
                   decltype(Container{parent, binding0})>,
      "parent and arg should produce single-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, Parent, void>,
                   decltype(Container{parent, Cache{}, binding0})>,
      "parent, cach, and arg should produce single-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent, binding0})>,
      "tag, parent, and arg should produce single-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0>, Cache, Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent, Cache{}, binding0})>,
      "tag, parent, cache, and arg should produce single-element Config");

  // Multiple-element ctors.

  static_assert(std::same_as<Container<Config<Binding0, Binding1, Binding2>,
                                       Cache, Dispatcher, void>,
                             decltype(Container{binding0, binding1, binding2})>,
                "multiple args should produce multiple-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Cache,
                             Dispatcher, void, Tag>,
                   decltype(Container{Tag{}, binding0, binding1, binding2})>,
      "tag and args should produce multiple-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Cache,
                             Dispatcher, Parent, void>,
                   decltype(Container{parent, binding0, binding1, binding2})>,
      "parent and args should produce multiple-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Cache,
                             Dispatcher, Parent, void>,
                   decltype(Container{parent, Cache{}, binding0, binding1,
                                      binding2})>,
      "parent, cache, and args should produce multiple-element Config");

  static_assert(std::same_as<Container<Config<Binding0, Binding1, Binding2>,
                                       Cache, Dispatcher, Parent, Tag>,
                             decltype(Container{Tag{}, parent, binding0,
                                                binding1, binding2})>,
                "tag, parent, and args should produce multiple-element Config");

  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Cache,
                             Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent, Cache{}, binding0,
                                      binding1, binding2})>,
      "tag, parent, cache, and args should produce multiple-element Config");

  // Type uniqueness.

  static_assert(
      !std::same_as<Container<Config<Binding0>, Cache, Dispatcher, void>,
                    Container<Config<Binding1>, Cache, Dispatcher, void>>,
      "different bindings should produce different containers");

  static_assert(
      !std::same_as<
          Container<Config<Binding0>, Cache, Dispatcher, void>,
          Container<Config<Binding1>, cache::Instance, Dispatcher, void>>,
      "different caches should produce different containers");

  static_assert(
      !std::same_as<Container<Config<Binding0>, Cache, Dispatcher, void>,
                    Container<Config<Binding0>, Cache, Dispatcher, Parent>>,
      "different nesting levels should produce different containers");

  static_assert(
      !std::same_as<
          Container<Config<Binding0>, Cache, Dispatcher, void, int_t>,
          Container<Config<Binding0>, Cache, Dispatcher, void, uint_t>>,
      "different tags should produce different containers");

  // Instance uniqueness.

  static_assert(!std::same_as<decltype(dink_unique_container()),
                              decltype(dink_unique_container())>,
                "instances should be unique");

  static_assert(!std::same_as<decltype(dink_unique_container(parent)),
                              decltype(dink_unique_container(parent))>,
                "instances should be unique");

  static_assert(!std::same_as<decltype(dink_unique_container(binding0)),
                              decltype(dink_unique_container(binding0))>,
                "instances should be unique");

  static_assert(
      !std::same_as<decltype(dink_unique_container(parent, binding0)),
                    decltype(dink_unique_container(parent, binding0))>,
      "instances should be unique");
};

struct ContainerTest : Test {
  using ParentBinding = Binding<int_t, scope::Transient, provider::Ctor<int_t>>;
  using ChildBinding =
      Binding<uint_t, scope::Transient, provider::Ctor<uint_t>>;

  struct Requested {
    using Id = int_t;
    Id id{};

    static constexpr auto expected_id = Id{3};
  };
  Requested requested;

  struct Cache {};

  struct MockDispatcher;
  struct Dispatcher {
    MockDispatcher* mock = nullptr;

    template <typename ActualRequested, typename Container, typename Config,
              typename ParentPtr>
    auto resolve(Container& container, Config& config, ParentPtr parent)
        -> meta::RemoveRvalueRef<ActualRequested> {
      if constexpr (std::same_as<ActualRequested, Requested>) {
        return mock->resolve_value(container, config, parent);
      } else if constexpr (std::same_as<ActualRequested, Requested&&>) {
        return mock->resolve_value(container, config, parent);
      } else if constexpr (std::same_as<ActualRequested, Requested&>) {
        if constexpr (std::same_as<ParentPtr, std::nullptr_t>) {
          return mock->resolve_reference(container, config, parent);
        } else {
          return mock->resolve_child(container, config, parent);
        }
      } else if constexpr (std::same_as<ActualRequested, Requested*>) {
        return &mock->resolve_reference(container, config, parent);
      }
    }
  };

  using ParentConfig = Config<ParentBinding>;
  using ChildConfig = Config<ChildBinding>;
  using Parent = Container<ParentConfig, Cache, Dispatcher>;
  using Child = Container<ChildConfig, Cache, Dispatcher, Parent>;

  struct MockDispatcher {
    MOCK_METHOD(Requested, resolve_value,
                (Parent & container, ParentConfig& config,
                 std::nullptr_t parent));
    MOCK_METHOD(Requested&, resolve_reference,
                (Parent & container, ParentConfig& config,
                 std::nullptr_t parent));
    MOCK_METHOD(Requested&, resolve_child,
                (Child & container, ChildConfig& config, Parent* parent));

    virtual ~MockDispatcher() = default;
  };
  StrictMock<MockDispatcher> mock_dispatcher;

  Parent parent{
      Cache{},
      Dispatcher{&mock_dispatcher},
      ParentConfig{ParentBinding{}},
  };
  Child child{parent, Cache{}, Dispatcher{&mock_dispatcher},
              ChildConfig{ChildBinding{}}};
};

TEST_F(ContainerTest, resolve_value) {
  EXPECT_CALL(mock_dispatcher,
              resolve_value(Ref(parent), A<ParentConfig&>(), nullptr))
      .WillOnce(Return(Requested{Requested::expected_id}));

  const auto result = parent.template resolve<Requested>();

  ASSERT_EQ(Requested::expected_id, result.id);
}

TEST_F(ContainerTest, resolve_rvalue_reference) {
  EXPECT_CALL(mock_dispatcher,
              resolve_value(Ref(parent), A<ParentConfig&>(), nullptr))
      .WillOnce(Return(Requested{Requested::expected_id}));

  const auto result = parent.template resolve<Requested&&>();

  ASSERT_EQ(Requested::expected_id, result.id);
}

TEST_F(ContainerTest, resolve_reference) {
  EXPECT_CALL(mock_dispatcher,
              resolve_reference(Ref(parent), A<ParentConfig&>(), nullptr))
      .WillOnce(ReturnRef(requested));

  auto& result = parent.template resolve<Requested&>();

  ASSERT_EQ(&result, &requested);
}

TEST_F(ContainerTest, resolve_pointer) {
  EXPECT_CALL(mock_dispatcher,
              resolve_reference(Ref(parent), A<ParentConfig&>(), nullptr))
      .WillOnce(ReturnRef(requested));

  auto* const result = parent.template resolve<Requested*>();

  ASSERT_EQ(result, &requested);
}

TEST_F(ContainerTest, resolve_child) {
  EXPECT_CALL(mock_dispatcher,
              resolve_child(Ref(child), A<ChildConfig&>(), &parent))
      .WillOnce(ReturnRef(requested));

  auto& result = child.template resolve<Requested&>();

  ASSERT_EQ(&result, &requested);
}

}  // namespace
}  // namespace dink
