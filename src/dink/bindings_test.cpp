/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/type_list.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace dink {

template <typename bindings_tuple_t>
struct get_from_types_f;

template <typename... bindings_t>
struct get_from_types_f<std::tuple<bindings_t...>>
{
    using type = type_list_t<typename bindings_t::from_t...>;
};

/*!
    extracts the source type (`from_t`) from each binding in a tuple

    Given a tuple of binding types, this alias produces a `type_list` containing the `from_t` member of each binding,
    preserving order.

    \tparam bindings_tuple_t a `std::tuple` of binding types to inspect.
*/
template <typename bindings_tuple_t>
using get_from_types_t = typename get_from_types_f<std::remove_cvref_t<bindings_tuple_t>>::type;

template <typename T> concept has_element_type = requires { typename T::element_type; };

template <typename T>
struct get_element_type_f
{
    using type = T;
};

template <typename T>
struct get_element_type_f<T*> : get_element_type_f<T>
{};

template <typename T>
requires has_element_type<T>
struct get_element_type_f<T> : get_element_type_f<typename T::element_type>
{};

template <typename T>
using get_element_type_t = typename get_element_type_f<std::remove_cvref_t<T>>::type;

template <typename T>
struct remove_reference_wrapper_f
{
    using type = T;
};

template <typename T>
struct remove_reference_wrapper_f<std::reference_wrapper<T>> : remove_reference_wrapper_f<T>
{};

template <typename T>
using remove_reference_wrapper_t = remove_reference_wrapper_f<T>::type;

template <typename T>
using remove_reference_t = remove_reference_wrapper_t<std::remove_reference_t<T>>;

template <typename T>
using get_underlying_t = std::remove_cv_t<get_element_type_t<remove_reference_t<T>>>;

template <typename binder_t>
struct binder_type_transform_t
{
    using type = binder_t;
};

struct default_provider_t
{
    template <typename resolved_t, typename container_t>
    auto get(container_t& container)
    {
        static constexpr auto const has_reference_semantics
            = std::is_lvalue_reference_v<resolved_t> || std::is_pointer_v<resolved_t>;
        if constexpr (has_reference_semantics)
        {
            using underlying_t = get_underlying_t<resolved_t>;

            auto* instance = container.template cached_instance<underlying_t>();
            if (!instance) instance = &container.cache_instance(container.template construct<resolved_t>());

            if constexpr (std::is_pointer_v<resolved_t>) { return *instance; }
            else { return instance; }
        }
        else { return container.template construct<resolved_t>(); }
    }
};

struct instance_cache_t
{
    struct cached_instance_t
    {
        using instance_ptr_t = std::unique_ptr<void, void (*)(void*)>;
        instance_ptr_t instance{nullptr, &default_typed_dtor};

        template <typename instance_t>
        static auto typed_dtor(void* instance) noexcept -> void
        {
            delete static_cast<instance_t*>(instance);
        }

        static auto default_typed_dtor(void*) noexcept -> void {}

        template <typename instance_t>
        explicit cached_instance_t(instance_t&& instance) noexcept
            : instance{instance_ptr_t{new instance_t{std::forward<instance_t>(instance)}, &typed_dtor<instance_t>}}
        {}

        cached_instance_t() noexcept = default;
    };

    std::unordered_map<std::type_index, cached_instance_t> instance_cache;

    template <typename underlying_t>
    auto cached_instance() const noexcept -> underlying_t*
    {
        auto result = instance_cache.find(std::type_index{typeid(underlying_t)});
        if (std::end(instance_cache) == result) return nullptr;
        return static_cast<underlying_t*>(result->second.instance.get());
    }

    template <typename underlying_t>
    auto cache_instance(underlying_t&& underlying) -> underlying_t&
    {
        assert(nullptr == cached_instance<underlying_t>());
        auto& result = instance_cache[std::type_index{typeid(underlying_t)}];
        result = cached_instance_t{std::forward<underlying_t>(underlying)};
        return *static_cast<underlying_t*>(result.instance.get());
    }
};

template <typename bindings_tuple_t, typename default_provider_t = default_provider_t>
struct container_t
{
    using from_types_t = get_from_types_t<bindings_tuple_t>;
    bindings_tuple_t bindings_tuple;
    default_provider_t default_provider;

    template <typename resolved_t>
    auto resolve() -> resolved_t
    {
        static auto constexpr bindings_type_index = type_list::index_of_v<from_types_t, resolved_t>;
        if constexpr (bindings_type_index != type_list::npos)
        {
            auto& provider = std::get<bindings_type_index>(bindings_tuple);
            return provider.resolve(*this);
        }
        else { return default_provider.template get<resolved_t>(*this); }
    }

    template <typename factory_t>
    auto resolve(factory_t&& factory)
    {
        return factory();
    }

    template <typename resolved_t>
    auto construct() -> resolved_t
    {
        return resolved_t{};
    }

    instance_cache_t instance_cache;

    template <typename underlying_t>
    auto cached_instance() const noexcept -> underlying_t*
    {
        return instance_cache.template cached_instance<underlying_t>();
    }

    template <typename underlying_t>
    auto cache_instance(underlying_t&& underlying) -> underlying_t&
    {
        return instance_cache.cache_instance(std::forward<underlying_t>(underlying));
    }

    explicit container_t(bindings_tuple_t bindings_tuple) noexcept : bindings_tuple{std::move(bindings_tuple)} {}

    template <typename... bindings_t>
    explicit container_t(bindings_t&&... bindings) noexcept : container_t{std::tuple{std::move(bindings)...}}
    {}
};

template <typename... bindings_t>
container_t(bindings_t&&... bindings) -> container_t<std::tuple<bindings_t...>>;

namespace providers {

template <typename resolved_t>
struct type_t
{
    template <typename container_t>
    constexpr auto get(container_t& container) const
    {
        return container.template construct<resolved_t>();
    }

    explicit type_t(std::in_place_t) noexcept {}
    explicit type_t() noexcept = default;
};

template <typename resolved_t>
struct instance_t
{
    mutable resolved_t instance;

    template <typename container_t>
    constexpr auto get(container_t&) const -> resolved_t&
    {
        return instance;
    }

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

    template <typename container_t>
    constexpr auto get(container_t&) const -> resolved_t&
    {
        return instance;
    }

    explicit instance_t(resolved_t& instance) noexcept : instance{instance} {}
    instance_t(std::in_place_t, resolved_t& instance) : instance_t{instance} {}
};

template <typename resolved_t>
struct prototype_t
{
    mutable resolved_t prototype;

    template <typename container_t>
    constexpr auto get(container_t&) const -> resolved_t
    {
        return prototype;
    }

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

    template <typename container_t>
    constexpr auto get(container_t&) const -> resolved_t
    {
        return prototype;
    }

    explicit prototype_t(resolved_t& prototype) noexcept : prototype{prototype} {}
    prototype_t(std::in_place_t, prototype_t& prototype) : prototype_t{prototype} {}
};

template <typename factory_p>
struct factory_t
{
    factory_p factory;

    using resolved_t = decltype(std::declval<factory_p>()());

    template <typename container_t>
    constexpr auto get(container_t& container) const -> resolved_t
    {
        return container.resolve(factory);
    }

    explicit factory_t(factory_p factory) noexcept : factory{std::move(factory)} {}

    template <typename... factory_args_t>
    explicit factory_t(std::in_place_t, factory_args_t&&... factory_args)
        : factory(std::forward<factory_args_t>(factory_args)...)
    {}
};

} // namespace providers

namespace scopes {

template <typename from_p, typename provider_t>
struct transient_t
{
    using from_t = from_p;

    provider_t provider;

    template <typename container_t>
    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    template <typename container_t>
    constexpr auto resolve(container_t& container) const -> resolved_t<container_t>
    {
        return provider.get(container);
    }
};

template <typename from_p, typename provider_t>
struct singleton_t
{
    using from_t = from_p;

    provider_t provider;

    template <typename container_t>
    using resolved_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    template <typename container_t>
    auto resolve(container_t& container) const -> resolved_t<container_t>&
    {
        /*  
            singletons use literal meyers singletons
            
            This is a deliberate design decision prioritizing lookup performance vs strictly binding lifetimes to the
            root container. When reflection makes a big tuple possible and we can store these there, they will move and
            lifetimes will match.
        */
        static decltype(auto) cache = provider.get(container);
        return cache;
    }
};

template <typename from_p, typename provider_t>
struct scoped_t
{
    using from_t = from_p;

    provider_t provider;

    template <typename container_t>
    using resolved_by_t = decltype(std::declval<provider_t>().get(std::declval<container_t&>()));

    template <typename container_t>
    auto resolve(container_t& container) const -> resolved_by_t<container_t>&
    {
        using resolved_t = resolved_by_t<container_t>;
        using underlying_t = get_underlying_t<resolved_t>;
        auto* instance = container.template cached_instance<underlying_t>();
        if (!instance) instance = &container.cache_instance(provider.get(container));
        if constexpr (std::is_pointer_v<remove_reference_t<resolved_t>>) { return instance; }
        else { return *instance; }
    }
};

} // namespace scopes

/*!
    binds provider to scope
    
    This type currently peforms two roles. Nominally, it is part of the builder pattern that caches the given provider,
    then moves it into the configured scope. In the case where no scope is configured, it behaves as the default,
    transient scope. This will be addressed in prod later, but for now, this is pragmatic.
    
    Fulfilling this dual role requires specializing in() on whether the instance is an rvalue or lvalue. For an rvalue,
    the provider can be moved, but for an lvalue, it must be copied. When the provider is instance or prototype, this
    forces a copy of the underlying, which is inefficient, and for the reference specializations, also changes the 
    reference.
*/
template <typename from_p, typename provider_t>
struct scope_binder_t
{
    using from_t = from_p;

    provider_t provider;

    template <template <typename, typename> class scope_t>
    constexpr auto in() const& -> scope_t<from_t, provider_t>
    {
        return scope_t<from_t, provider_t>{provider};
    }

    template <template <typename, typename> class scope_t>
    constexpr auto in() && noexcept -> scope_t<from_t, provider_t>
    {
        return scope_t<from_t, provider_t>{std::move(provider)};
    }

    // if scope is not specified by a call to in(), masquerade as a transient scope
    template <typename container_t>
    constexpr auto resolve(container_t& container) const -> auto
    {
        return provider.get(container);
    }
};

template <typename from_p>
struct provider_binder_t
{
    using from_t = from_p;

    template <typename to_t>
    constexpr auto to() const noexcept -> scope_binder_t<from_t, providers::type_t<to_t>>
    {
        return scope_binder_t<from_t, providers::type_t<to_t>>{providers::type_t<to_t>{}};
    }

    template <template <typename> class provider_t, typename provider_payload_t>
    constexpr auto to(provider_payload_t&& provider_payload) const noexcept
    {
        return scope_binder_t<from_t, provider_t<std::decay_t<provider_payload_t>>>{
            provider_t{std::forward<provider_payload_t>(provider_payload)}
        };
    }

    template <template <typename> class provider_t, typename provider_payload_t, typename... payload_args_t>
    constexpr auto to(payload_args_t&&... payload_args) const noexcept
    {
        return scope_binder_t<from_t, provider_t<provider_payload_t>>{
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
    service_a_t() { std::cerr << "    > service_a constructor called.\n"; }
    std::string id() const override { return "service_a"; }
};

struct service_b_t : service_i
{
    service_b_t() { std::cerr << "    > service_b constructor called.\n"; }
    std::string id() const override { return "service_b"; }
};

struct config_t
{
    int value;

    config_t(int v = 0) : value(v)
    {
        std::cerr << "    > config_t constructor called. value=" << value << ", this=" << this << "\n";
    }

    config_t(config_t const& other) : value(other.value)
    {
        std::cerr
            << "    > config_t copy constructor called from "
            << &other
            << " to "
            << this
            << ". value="
            << value
            << "\n";
    }

    config_t(config_t&& other) : value(std::move(other).value)
    {
        std::cerr
            << "    > config_t move constructor called from "
            << &other
            << " to "
            << this
            << ". value="
            << value
            << "\n";
    }
};

namespace {

TEST(binding, original_examples)
{
    auto container = container_t{};

    std::cerr << "--- binding 1: type binding to service_a_t (singleton) ---\n";
    auto binding1 = bind<service_i>().template to<service_a_t>().in<scopes::singleton_t>();
    std::cerr << "resolving 1st time... address: " << &binding1.resolve(container) << "\n";
    std::cerr << "resolving 2nd time... address: " << &binding1.resolve(container) << "\n";

    std::cerr << "\n--- binding 2: instance binding with a shared_ptr (inherently singleton-like) ---\n";
    auto instance_of_b = std::make_shared<service_b_t>();
    auto binding2
        = bind<std::shared_ptr<service_i>>().to<providers::instance_t>(instance_of_b).in<scopes::singleton_t>();
    std::cerr << "resolving 1st time... address: " << &binding2.resolve(container) << "\n";
    std::cerr << "resolving 2nd time... address: " << &binding2.resolve(container) << "\n";

    std::cerr << "\n--- binding 3: type binding to service_a (transient) ---\n";
    auto binding3 = bind<service_i>().template to<service_a_t>();
    std::cerr << "resolving 1st time...\n";
    binding3.resolve(container);
    std::cerr << "resolving 2nd time...\n";
    binding3.resolve(container);

    std::cerr << "\n--- binding 4: factory binding (transient by default) ---\n";
    auto binding4 = bind<std::unique_ptr<service_i>>().to<providers::factory_t>([] {
        return std::make_unique<service_a_t>();
    });
    std::cerr << "resolving 1st time...\n";
    binding4.resolve(container);
    std::cerr << "resolving 2nd time...\n";
    binding4.resolve(container);

    static_assert(
        std::is_same_v<
            get_from_types_t<decltype(std::tuple{binding1, binding2, binding3, binding4})>,
            type_list_t<service_i, std::shared_ptr<service_i>, service_i, std::unique_ptr<service_i>>>
    );
}

TEST(provider, instance_and_prototype)
{
    auto container = container_t{};

    std::cerr << "\n--- 1. shared internal copy (to_instance(...).in_singleton()) ---\n";
    {
        config_t initial_config{100};
        auto binding = bind<config_t>().to<providers::instance_t>(initial_config).in<scopes::singleton_t>();
        std::cerr << "binding created with an internal copy of initial_config.\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cerr << "resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cerr << "resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cerr << "\n--- 2. shared external reference (to_instance(std::ref(...)).in_singleton()) ---\n";
    {
        config_t external_config{200};
        auto binding = bind<config_t>().to<providers::instance_t>(std::ref(external_config)).in<scopes::singleton_t>();
        std::cerr << "binding created with a reference to external_config (" << &external_config << ").\n";

        auto& c1 = binding.resolve(container);
        auto& c2 = binding.resolve(container);
        std::cerr << "resolved 1st time: " << &c1 << " (value=" << c1.value << ")\n";
        std::cerr << "resolved 2nd time: " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cerr << "\n--- 3. transient from internal copy (to_prototype(...).in_transient()) ---\n";
    {
        auto const initial_config = config_t{300};
        auto binding = bind<config_t>().to<providers::prototype_t>(initial_config).in<scopes::transient_t>();
        std::cerr << "binding created with an internal copy of prototype_config.\n";

        auto c1 = binding.resolve(container);
        auto c2 = binding.resolve(container);
        std::cerr << "resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";
        std::cerr << "resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }

    std::cerr << "\n--- 4. transient from external reference (to_prototype(std::ref(...)).in_transient()) ---\n";
    {
        config_t external_prototype{400};
        auto binding = bind<config_t>().to<providers::prototype_t>(std::ref(external_prototype));
        std::cerr << "binding created with a reference to external_prototype.\n";

        std::cerr
            << "original prototype at "
            << &external_prototype
            << " has value "
            << external_prototype.value
            << ".\n";

        auto c1 = binding.resolve(container);
        std::cerr << "resolved 1st time: object at " << &c1 << " (value=" << c1.value << ")\n";

        // modify the external prototype to prove we're copying from the live object.
        external_prototype.value = 401;
        std::cerr << "modified original prototype value to 401.\n";

        auto c2 = binding.resolve(container);
        std::cerr << "resolved 2nd time: object at " << &c2 << " (value=" << c2.value << ")\n";
    }
}

} // namespace
} // namespace dink
