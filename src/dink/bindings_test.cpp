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

// A stand-in until container is ready or this is no longer a proof of concept.
struct container_t
{
    template <typename resolved_t>
    auto resolve() -> resolved_t
    {
        return resolved_t{};
    }

    template <typename factory_t>
    auto resolve(factory_t&& factory)
    {
        return factory();
    }
};

namespace providers {

template <typename resolved_t>
struct type_t
{
    constexpr auto get(container_t& container) const { return container.template resolve<resolved_t>(); }

    explicit type_t(std::in_place_t) noexcept {}
    explicit type_t() noexcept = default;
};

template <typename resolved_t>
struct instance_t
{
    mutable resolved_t instance;

    constexpr auto get(container_t&) const -> resolved_t& { return instance; }

    explicit instance_t(resolved_t instance) noexcept : instance{std::move(instance)} {}

    template <typename... instance_args_t>
    explicit instance_t(std::in_place_t, instance_args_t&&... instance_args)
        : instance(std::forward<instance_args_t>(instance_args)...)
    {}
};

template <typename resolved_t>
struct instance_t<std::reference_wrapper<resolved_t>>
{
    resolved_t& instance;

    constexpr auto get(container_t&) const -> resolved_t& { return instance; }

    explicit instance_t(resolved_t& instance) noexcept : instance{instance} {}
    instance_t(std::in_place_t, resolved_t& instance) : instance_t{instance} {}
};

template <typename resolved_t>
struct prototype_t
{
    mutable resolved_t prototype;

    constexpr auto get(container_t&) const -> resolved_t { return prototype; }

    explicit prototype_t(resolved_t prototype) noexcept : prototype{std::move(prototype)} {}

    template <typename... prototype_args_t>
    explicit prototype_t(std::in_place_t, prototype_args_t&&... prototype_args)
        : prototype(std::forward<prototype_args_t>(prototype_args)...)
    {}
};

template <typename resolved_t>
struct prototype_t<std::reference_wrapper<resolved_t>>
{
    resolved_t& prototype;

    constexpr auto get(container_t&) const -> resolved_t { return prototype; }

    explicit prototype_t(resolved_t& prototype) noexcept : prototype{prototype} {}
    prototype_t(std::in_place_t, prototype_t& prototype) : prototype_t{prototype} {}
};

template <typename factory_p>
struct factory_t
{
    factory_p factory;

    using resolved_t = decltype(std::declval<factory_p>()());
    constexpr auto get(container_t& container) const -> resolved_t { return container.resolve(factory); }

    explicit factory_t(factory_p factory) noexcept : factory{std::move(factory)} {}

    template <typename... factory_args_t>
    explicit factory_t(std::in_place_t, factory_args_t&&... factory_args)
        : factory(std::forward<factory_args_t>(factory_args)...)
    {}
};

} // namespace providers

namespace scopes {

template <typename from_t, typename provider_t>
struct transient_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    constexpr auto resolve(container_t& container) const -> resolved_t { return provider.get(container); }
};

template <typename from_t, typename provider_t>
struct singleton_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    auto resolve(container_t& container) const -> resolved_t&
    {
        /*  
            singletons use literal meyers singletons
            
            This is a deliberate design decision prioritizing lookup performance vs strictly binding lifetimes to the
            root container. When reflection makes a big tuple possible and we can store these there, they will move and
            lifetimes will match.
        */
        static auto cache = provider.get(container);
        return cache;
    }
};

template <typename from_t, typename provider_t>
struct scoped_t
{
    provider_t provider;

    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    auto resolve(container_t& container) const -> resolved_t&
    {
        /*
            placeholder; this will eventually use a hetero hash table
            
            The cache is in the container, so this will ask the container for the cached instance.
            If the container finds a cached instance, it returns it.
            If not, the provider is used to create the instance, which is stored in the container, then returned from
            the container's instance to provided a stable reference.
        */
        static auto cache = provider.get(container);
        return cache;
    }
};

} // namespace scopes

template <typename from_t, typename provider_t>
struct lifetime_binder_t
{
    provider_t provider;

    template <template <typename, typename> class lifetime_t>
    constexpr auto in() const -> lifetime_t<from_t, provider_t>
    {
        return lifetime_t<from_t, provider_t>{provider};
    }

    // if lifetime is not specified by a call to in(), masquerade as a transient lifetime
    constexpr auto resolve(container_t& container) const -> auto
    {
        return scopes::transient_t<from_t, provider_t>{provider}.resolve(container);
    }
};

template <typename from_t>
struct provider_binder_t
{
    template <typename to_t>
    constexpr auto to() const noexcept -> lifetime_binder_t<from_t, providers::type_t<to_t>>
    {
        return lifetime_binder_t<from_t, providers::type_t<to_t>>{providers::type_t<to_t>{}};
    }

    template <template <typename> class provider_t, typename provider_payload_t>
    constexpr auto to(provider_payload_t&& provider_payload) const noexcept
    {
        return lifetime_binder_t<from_t, provider_t<std::decay_t<provider_payload_t>>>{
            provider_t{std::forward<provider_payload_t>(provider_payload)}
        };
    }

    template <template <typename> class provider_t, typename provider_payload_t, typename... payload_args_t>
    constexpr auto to(payload_args_t&&... payload_args) const noexcept
    {
        return lifetime_binder_t<from_t, provider_t<provider_payload_t>>{
            provider_t{std::in_place, std::forward<payload_args_t>(payload_args)...}
        };
    }
};

template <typename from_t>
constexpr auto bind() noexcept -> provider_binder_t<from_t>
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
    service_a_t() { std::cout << "    > service_a constructor called.\n"; }
    std::string id() const override { return "service_a"; }
};

struct service_b_t : service_i
{
    service_b_t() { std::cout << "    > service_b constructor called.\n"; }
    std::string id() const override { return "service_b"; }
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

namespace {

TEST(binding, original_examples)
{
    auto container = container_t{};

    std::cout << "--- binding 1: type binding to service_a_t (singleton) ---\n";
    auto binding1 = bind<service_i>().template to<service_a_t>().in<scopes::singleton_t>();
    std::cout << "resolving 1st time... address: " << &binding1.resolve(container) << "\n";
    std::cout << "resolving 2nd time... address: " << &binding1.resolve(container) << "\n";

    std::cout << "\n--- binding 2: instance binding with a shared_ptr (inherently singleton-like) ---\n";
    auto instance_of_b = std::make_shared<service_b_t>();
    auto binding2
        = bind<std::shared_ptr<service_i>>().to<providers::instance_t>(instance_of_b).in<scopes::singleton_t>();
    std::cout << "resolving 1st time... address: " << &binding2.resolve(container) << "\n";
    std::cout << "resolving 2nd time... address: " << &binding2.resolve(container) << "\n";

    std::cout << "\n--- binding 3: type binding to service_a (transient) ---\n";
    auto binding3 = bind<service_i>().template to<service_a_t>();
    std::cout << "resolving 1st time...\n";
    binding3.resolve(container);
    std::cout << "resolving 2nd time...\n";
    binding3.resolve(container);

    std::cout << "\n--- binding 4: factory binding (transient by default) ---\n";
    auto binding4 = bind<std::unique_ptr<service_i>>().to<providers::factory_t>([] {
        return std::make_unique<service_a_t>();
    });
    std::cout << "resolving 1st time...\n";
    binding4.resolve(container);
    std::cout << "resolving 2nd time...\n";
    binding4.resolve(container);
}

TEST(provider, instance_and_prototype)
{
    auto container = container_t{};

    std::cout << "\n--- 1. shared internal copy (to_instance(...).in_singleton()) ---\n";
    {
        config_t initial_config{100};
        auto binding = bind<config_t>().to<providers::instance_t>(initial_config).in<scopes::singleton_t>();
        std::cout << "binding created with an internal copy of initial_config.\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cout << "resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 2. shared external reference (to_instance(std::ref(...)).in_singleton()) ---\n";
    {
        config_t external_config{200};
        auto binding = bind<config_t>().to<providers::instance_t>(std::ref(external_config)).in<scopes::scoped_t>();
        std::cout << "binding created with a reference to external_config (" << &external_config << ").\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cout << "resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 3. transient from internal copy (to_prototype(...).in_transient()) ---\n";
    {
        config_t prototype_config{300};
        auto binding = bind<config_t>().to<providers::prototype_t>(prototype_config).in<scopes::transient_t>();
        std::cout << "binding created with an internal copy of prototype_config.\n";

        auto c1 = binding.resolve(container);
        auto c2 = binding.resolve(container);
        std::cout << "resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";
        std::cout << "resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cout << "\n--- 4. transient from external reference (to_prototype(std::ref(...)).in_transient()) ---\n";
    {
        config_t external_prototype{400};
        auto binding = bind<config_t>().to<providers::prototype_t>(std::ref(external_prototype));
        std::cout << "binding created with a reference to external_prototype.\n";

        std::cout
            << "original prototype at "
            << &external_prototype
            << " has value "
            << external_prototype.value
            << ".\n";

        auto c1 = binding.resolve(container);
        std::cout << "resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";

        // modify the external prototype to prove we're copying from the live object.
        external_prototype.value = 401;
        std::cout << "modified original prototype value to 401.\n";

        auto c2 = binding.resolve(container);
        std::cout << "resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }
}

} // namespace
} // namespace dink
