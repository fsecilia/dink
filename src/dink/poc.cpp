/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace dink {
namespace {

// A mock container to stand in for the real injector.
struct container_t
{
    template <typename resolved_t>
    auto resolve() -> resolved_t
    {
        return resolved_t{};
    }
};

// --- Providers (Creation Strategies) ---

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
        std::cout << "  Returning ref to pre-existing internal instance via instance_provider_t<instance_t>.\n";
        return instance;
    }
};

template <typename instance_t>
struct instance_provider_t<std::reference_wrapper<instance_t>>
{
    instance_t& instance;

    constexpr auto get(container_t&) const -> instance_t&
    {
        std::cout
            << "  Returning ref to pre-existing external instance via instance_provider_t<std::reference_wrapper<instance_t>>.\n";
        return instance;
    }
};

template <typename prototype_t>
struct prototype_provider_t
{
    mutable prototype_t prototype;

    constexpr auto get(container_t&) const -> prototype_t
    {
        std::cout << "  Returning new copy of pre-existing internal prototype via prototype_provider_t<prototype_t>.\n";
        return prototype;
    }
};

template <typename prototype_t>
struct prototype_provider_t<std::reference_wrapper<prototype_t>>
{
    prototype_t& prototype;

    constexpr auto get(container_t&) const -> prototype_t
    {
        std::cout
            << "  Returning new copy of pre-existing external prototype ref via prototype_provider_t<std::reference_wrapper<prototype_t>>.\n";
        return prototype;
    }
};

template <typename factory_t>
struct factory_provider_t
{
    factory_t factory;

    using resolved_t = decltype(std::declval<factory_t>()());

    constexpr auto get(container_t&) const -> resolved_t
    {
        std::cout << "  Invoking factory function via factory_provider_t.\n";
        return factory();
    }
};

// --- Scopes (Lifetime Managers) ---

template <typename from_t, typename provider_t>
struct transient_binding_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    constexpr auto resolve(container_t& container) const -> resolved_t
    {
        std::cout << " transient scope\n";
        return provider.get(container);
    }
};

template <typename from_t, typename provider_t>
struct singleton_binding_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    auto resolve(container_t& container) const -> resolved_t&
    {
        std::cout << " singleton scope\n";
        // NOTE: This Meyer's singleton is a placeholder for the real injector's tuple cache.
        static auto cache = provider.get(container);
        return cache;
    }
};

// --- Fluent API ---

template <typename from_t>
struct bind_start_t;

template <typename from_t, typename provider_t>
struct binding_builder_t
{
    provider_t provider;

    constexpr auto in_transient() const -> transient_binding_t<from_t, provider_t> { return {provider}; }
    constexpr auto in_singleton() const -> singleton_binding_t<from_t, provider_t> { return {provider}; }

    // Default to transient if no scope is specified
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
        using provider_arg_t = std::decay_t<instance_t>;
        return binding_builder_t<from_t, instance_provider_t<provider_arg_t>>{{std::forward<instance_t>(instance)}};
    }

    template <typename prototype_t>
    constexpr auto to_prototype(prototype_t&& prototype) const noexcept
    {
        using provider_arg_t = std::decay_t<prototype_t>;
        return binding_builder_t<from_t, prototype_provider_t<provider_arg_t>>{{std::forward<prototype_t>(prototype)}};
    }

    template <typename factory_t>
    constexpr auto to_factory(factory_t&& factory) const noexcept
    {
        using provider_arg_t = std::decay_t<factory_t>;
        return binding_builder_t<from_t, factory_provider_t<provider_arg_t>>{{std::forward<factory_t>(factory)}};
    }
};

template <typename from_t>
constexpr auto bind() noexcept -> bind_start_t<from_t>
{
    return {};
}

// --- Example Services ---

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

struct config_t
{
    int value;

    config_t(int v = 0) : value(v)
    {
        std::cout << "    > config_t constructor called. value=" << value << ", this=" << this << "\n";
    }

    config_t(config_t const& other) : value(other.value)
    {
        std::cout
            << "    > config_t copy constructor called from "
            << &other
            << " to "
            << this
            << ". value="
            << value
            << "\n";
    }

    config_t(config_t&&) = default;
};

// --- Tests ---

TEST(binding, original_examples)
{
    auto container = container_t{};

    std::cout << "--- binding 1: type binding to service_a_t (singleton) ---\n";
    auto binding1 = bind<service_i>().template to<service_a_t>().in_singleton();
    std::cout << "resolving 1st time... address: " << &binding1.resolve(container) << "\n";
    std::cout << "resolving 2nd time... address: " << &binding1.resolve(container) << "\n";

    std::cout << "\n--- binding 2: instance binding with a shared_ptr (inherently singleton-like) ---\n";
    auto instance_of_b = std::make_shared<service_b_t>();
    auto binding2 = bind<std::shared_ptr<service_i>>().to_instance(instance_of_b).in_singleton();
    std::cout << "resolving 1st time... address: " << &binding2.resolve(container) << "\n";
    std::cout << "resolving 2nd time... address: " << &binding2.resolve(container) << "\n";

    std::cout << "\n--- binding 3: type binding to service_a (transient) ---\n";
    auto binding3 = bind<service_i>().template to<service_a_t>().in_transient();
    std::cout << "resolving 1st time...\n";
    binding3.resolve(container);
    std::cout << "resolving 2nd time...\n";
    binding3.resolve(container);

    std::cout << "\n--- binding 4: factory binding (transient by default) ---\n";
    auto binding4 = bind<std::unique_ptr<service_i>>().to_factory([] { return std::make_unique<service_a_t>(); });
    std::cout << "resolving 1st time...\n";
    binding4.resolve(container);
    std::cout << "resolving 2nd time...\n";
    binding4.resolve(container);
}

TEST(provider, instance_and_prototype)
{
    auto container = container_t{};

    std::cout << "\n--- 1. Shared Internal Copy (to_instance(...).in_singleton()) ---\n";
    {
        config_t initial_config{100};
        auto binding = bind<config_t>().to_instance(initial_config).in_singleton();
        std::cout << "Binding created with an internal copy of initial_config.\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cout << "Resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "Resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 2. Shared External Reference (to_instance(std::ref(...)).in_singleton()) ---\n";
    {
        config_t external_config{200};
        auto binding = bind<config_t>().to_instance(std::ref(external_config)).in_singleton();
        std::cout << "Binding created with a reference to external_config (" << &external_config << ").\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cout << "Resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "Resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 3. Transient From Internal Copy (to_prototype(...).in_transient()) ---\n";
    {
        config_t prototype_config{300};
        auto binding = bind<config_t>().to_prototype(prototype_config).in_transient();
        std::cout << "Binding created with an internal copy of prototype_config.\n";

        auto c1 = binding.resolve(container);
        auto c2 = binding.resolve(container);
        std::cout << "Resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "Resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 4. Transient From External Reference (to_prototype(std::ref(...)).in_transient()) ---\n";
    {
        config_t external_prototype{400};
        auto binding = bind<config_t>().to_prototype(std::ref(external_prototype)).in_transient();
        std::cout << "Binding created with a reference to external_prototype.\n";

        std::cout
            << "Original prototype at "
            << &external_prototype
            << " has value "
            << external_prototype.value
            << ".\n";

        auto c1 = binding.resolve(container);
        std::cout << "Resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";

        // Modify the external prototype to prove we're copying from the live object.
        external_prototype.value = 401;
        std::cout << "Modified original prototype value to 401.\n";

        auto c2 = binding.resolve(container);
        std::cout << "Resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }
}

} // namespace
} // namespace dink
