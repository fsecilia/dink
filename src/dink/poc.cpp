/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace dink {
namespace {

struct container_t
{
    template <typename resolved_t>
    auto resolve() -> resolved_t
    {
        return resolved_t{};
    }
};

template <typename type_t>
struct type_provider_t
{
    constexpr auto get(container_t& container) const
    {
        std::cout << "  Resolving new instance of a specific type via type_provider_t.\n";
        return container.template resolve<type_t>();
    }
};

template <typename instance_t>
struct instance_provider_t
{
    mutable instance_t instance;

    constexpr auto get(container_t&) const -> instance_t&
    {
        std::cout << "  Returning pre-existing instance via instance_provider_t.\n";
        return instance;
    }
};

template <typename factory_t>
struct factory_provider_t
{
    factory_t factory;

    constexpr auto get(container_t&) const
    {
        std::cout << "  Invoking factory function via FactoryProvider.\n";
        return factory();
    }
};

template <typename from_t, typename provider_t>
struct transient_binding_t
{
    provider_t provider;
    constexpr auto resolve(container_t& container) const { return provider.get(container); }
};

template <typename from_t, typename provider_t>
struct singleton_binding_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    auto resolve(container_t& container) const -> resolved_t&
    {
        static auto cache = provider.get(container);
        return cache;
    }
};

template <typename from_t>
struct bind_start_t;

template <typename from_t, typename provider_t>
struct binding_builder_t
{
    provider_t provider;

    constexpr auto in_transient() const -> transient_binding_t<from_t, provider_t> { return {provider}; }
    constexpr auto in_singleton() const -> singleton_binding_t<from_t, provider_t> { return {provider}; }
    constexpr auto resolve(container_t& container) const -> auto { return in_transient().resolve(container); }
};

template <typename from_t>
struct bind_start_t
{
    template <typename to_t>
    constexpr auto to() const noexcept -> binding_builder_t<from_t, type_provider_t<to_t>>
    {
        return {type_provider_t<to_t>{}};
    }

    template <typename instance_t>
    constexpr auto to_instance(instance_t&& instance) const noexcept
    {
        using stored_t = std::decay_t<instance_t>;
        return binding_builder_t<from_t, instance_provider_t<stored_t>>{
            instance_provider_t<stored_t>{std::forward<instance_t>(instance)}
        };
    }

    template <typename factory_t>
    constexpr auto to_factory(factory_t&& factory) const noexcept
    {
        using stored_t = std::decay_t<factory_t>;
        return binding_builder_t<from_t, factory_provider_t<stored_t>>{
            factory_provider_t<stored_t>{std::forward<factory_t>(factory)}
        };
    }
};

template <typename from_t>
constexpr auto bind() noexcept -> bind_start_t<from_t>
{
    return {};
}

struct service_i
{
    virtual ~service_i() = default;
    virtual std::string id() const = 0;
};

struct service_a_t : service_i
{
    service_a_t() { std::cout << "    > ServiceA constructor called.\n"; }
    std::string id() const override { return "ServiceA"; }
};

struct service_b_t : service_i
{
    service_b_t() { std::cout << "    > ServiceB constructor called.\n"; }
    std::string id() const override { return "ServiceB"; }
};

TEST(bnding, example)
{
    auto container = container_t{};

    std::cout << "--- binding 1: type binding to service_a_t (singleton) ---\n";
    auto binding1 = bind<service_i>().template to<service_a_t>().in_singleton();
    std::cout << "resolving 1st time... " << &binding1.resolve(container) << "\n"; // first call: resolves and caches
    std::cout << "resolving 2nd time... " << &binding1.resolve(container) << "\n"; // second call: returns from cache

    std::cout << "\n--- binding 2: instance binding with a shared_ptr (inherently singleton-like) ---\n";
    auto instance_of_b = std::make_shared<service_b_t>();
    // the fluent chain .to_instance(...).in_singleton() now works consistently.
    auto binding2 = bind<std::shared_ptr<service_i>>().to_instance(instance_of_b).in_singleton();
    std::cout << "resolving 1st time... " << &binding2.resolve(container) << "\n";
    std::cout << "resolving 2nd time... " << &binding2.resolve(container) << "\n";

    std::cout << "\n--- binding 3: type binding to service_a (transient) ---\n";
    auto binding3 = bind<service_i>().template to<service_a_t>().in_transient();
    std::cout << "resolving 1st time.../n";
    binding3.resolve(container); // first call: resolves new instance
    std::cout << "resolving 2nd time.../n";
    binding3.resolve(container); // second call: resolves another new instance

    std::cout << "\n--- binding 4: factory binding (transient by default) ---\n";
    auto binding4 = bind<std::unique_ptr<service_i>>().to_factory([] { return std::make_unique<service_a_t>(); });
    std::cout << "resolving 1st time.../n";
    binding4.resolve(container);
    std::cout << "resolving 2nd time.../n";
    binding4.resolve(container);
}

} // namespace
} // namespace dink
