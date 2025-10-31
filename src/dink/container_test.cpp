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

  using Dispatcher = Dispatcher<>;
  using Parent = Container<Config<>, Dispatcher, void>;
  Parent parent;

  struct Tag {};

  // Empty ctors.
  static_assert(std::same_as<Container<Config<>, Dispatcher, void>,
                             decltype(Container{})>,
                "empty args should produce empty Config");
  static_assert(std::same_as<Container<Config<>, Dispatcher, void, Tag>,
                             decltype(Container{Tag{}})>,
                "tag should produce empty Config");
  static_assert(std::same_as<Container<Config<>, Dispatcher, Parent, void>,
                             decltype(Container{parent})>,
                "parent should produce empty Config");
  static_assert(std::same_as<Container<Config<>, Dispatcher, Parent, Tag>,
                             decltype(Container{Tag{}, parent})>,
                "tag and parent should produce empty Config");

  // Single-element ctors.
  static_assert(std::same_as<Container<Config<Binding0>, Dispatcher, void>,
                             decltype(Container{binding0})>,
                "single arg should produce single-element Config");
  static_assert(std::same_as<Container<Config<Binding0>, Dispatcher, void, Tag>,
                             decltype(Container{Tag{}, binding0})>,
                "tag and single arg should produce single-element Config");
  static_assert(
      std::same_as<Container<Config<Binding0>, Dispatcher, Parent, void>,
                   decltype(Container{parent, binding0})>,
      "parent and arg should produce single-element Config");
  static_assert(
      std::same_as<Container<Config<Binding0>, Dispatcher, Parent, Tag>,
                   decltype(Container{Tag{}, parent, binding0})>,
      "tag, parent, and arg should produce single-element Config");

  // Multiple-element ctors.
  static_assert(std::same_as<Container<Config<Binding0, Binding1, Binding2>,
                                       Dispatcher, void>,
                             decltype(Container{binding0, binding1, binding2})>,
                "multiple args should produce multiple-element Config");
  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Dispatcher,
                             void, Tag>,
                   decltype(Container{Tag{}, binding0, binding1, binding2})>,
      "tag and args should produce multiple-element Config");
  static_assert(
      std::same_as<Container<Config<Binding0, Binding1, Binding2>, Dispatcher,
                             Parent, void>,
                   decltype(Container{parent, binding0, binding1, binding2})>,
      "parent and args should produce multiple-element Config");
  static_assert(std::same_as<Container<Config<Binding0, Binding1, Binding2>,
                                       Dispatcher, Parent, Tag>,
                             decltype(Container{Tag{}, parent, binding0,
                                                binding1, binding2})>,
                "tag, parent, and args should produce multiple-element Config");

  // Type uniqueness.
  static_assert(!std::same_as<Container<Config<Binding0>, Dispatcher, void>,
                              Container<Config<Binding1>, Dispatcher, void>>,
                "different bindings should produce different containers");

  static_assert(!std::same_as<Container<Config<Binding0>, Dispatcher, void>,
                              Container<Config<Binding0>, Dispatcher, Parent>>,
                "different nesting levels should produce different containers");

  static_assert(
      !std::same_as<Container<Config<Binding0>, Dispatcher, void, int_t>,
                    Container<Config<Binding0>, Dispatcher, void, uint_t>>,
      "different tags should produce different containers");
};

struct ContainerTest : Test {
  using ParentBinding = Binding<int_t, scope::Transient, provider::Ctor<int_t>>;
  using ChildBinding =
      Binding<uint_t, scope::Transient, provider::Ctor<uint_t>>;

  struct Requested {};
  Requested requested;

  struct MockDispatcher;
  struct Dispatcher {
    MockDispatcher* mock = nullptr;

    template <typename Requested, typename Container, typename Config,
              typename ParentPtr>
    auto resolve(Container& container, Config& config, ParentPtr parent)
        -> remove_rvalue_ref_t<Requested> {
      return mock->resolve(container, config, parent);
    }
  };

  using ParentConfig = Config<ParentBinding>;
  using ChildConfig = Config<ChildBinding>;
  using Parent = Container<ParentConfig, Dispatcher>;
  using Child = Container<ChildConfig, Dispatcher, Parent>;

  struct MockDispatcher {
    MOCK_METHOD(Requested&, resolve,
                (Parent & container, ParentConfig& config,
                 std::nullptr_t parent));
    MOCK_METHOD(Requested&, resolve,
                (Child & container, ChildConfig& config, Parent* parent));

    virtual ~MockDispatcher() = default;
  };
  StrictMock<MockDispatcher> mock_dispatcher;

  Parent parent{ParentConfig{ParentBinding{}}, Dispatcher{&mock_dispatcher}};
  Child child{parent, ChildConfig{ChildBinding{}},
              Dispatcher{&mock_dispatcher}};
};

TEST_F(ContainerTest, resolve_parent) {
  EXPECT_CALL(mock_dispatcher,
              resolve(Ref(parent), A<ParentConfig&>(), nullptr))
      .WillOnce(ReturnRef(requested));

  auto& result = parent.template resolve<Requested&>();

  ASSERT_EQ(&result, &requested);
}

TEST_F(ContainerTest, resolve_child) {
  EXPECT_CALL(mock_dispatcher, resolve(Ref(child), A<ChildConfig&>(), &parent))
      .WillOnce(ReturnRef(requested));

  auto& result = child.template resolve<Requested&>();

  ASSERT_EQ(&result, &requested);
}

}  // namespace
}  // namespace dink
