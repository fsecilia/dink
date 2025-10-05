/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/instance_cache.hpp>
#include <dink/type_list.hpp>
#include <dink/unqualified.hpp>
#include <memory>
#include <type_traits>

namespace dink {

template <typename bindings_tuple_t>
struct get_key_types_f;

template <typename... bindings_t>
struct get_key_types_f<std::tuple<bindings_t...>>
{
    using type = type_list_t<typename bindings_t::interface_t...>;
};

/*!
    extracts the key type (`interface_t`) from each binding in a tuple

    Given a tuple of binding types, this alias produces a `type_list` containing the `interface_t` member of each
    binding, preserving order.

    \tparam bindings_tuple_t a `std::tuple` of binding types to inspect.
*/
template <typename bindings_tuple_t>
using get_key_types_t = typename get_key_types_f<std::remove_cvref_t<bindings_tuple_t>>::type;

template <typename type_t>
using key_t = unqualified_t<type_t>;

template <typename value_t>
struct is_unique_ptr_f : std::false_type
{};

template <typename value_t, typename deleter_t>
struct is_unique_ptr_f<std::unique_ptr<value_t, deleter_t>> : std::true_type
{};

template <typename value_t>
static inline constexpr auto is_unique_ptr_v = is_unique_ptr_f<value_t>::value;

template <typename value_t>
struct is_shared_ptr_f : std::false_type
{};

template <typename value_t>
struct is_shared_ptr_f<std::shared_ptr<value_t>> : std::true_type
{};

template <typename value_t>
static inline constexpr auto is_shared_ptr_v = is_shared_ptr_f<value_t>::value;

template <typename value_t>
struct is_weak_ptr_f : std::false_type
{};

template <typename value_t>
struct is_weak_ptr_f<std::weak_ptr<value_t>> : std::true_type
{};

template <typename value_t>
static inline constexpr auto is_weak_ptr_v = is_weak_ptr_f<value_t>::value;

template <typename value_t>
struct is_smart_ptr_f : std::disjunction<is_unique_ptr_f<value_t>, is_shared_ptr_f<value_t>, is_weak_ptr_f<value_t>>
{};

template <typename value_t>
static inline constexpr auto is_smart_ptr_v = is_smart_ptr_f<value_t>::value;

// a metafunction to find the core type for caching
template <typename t_p>
struct canonical_type_f;

// specialization for pointers
template <typename t_p>
struct canonical_type_f<t_p*>
{
    using type = typename canonical_type_f<t_p>::type;
};

// specialization for references
template <typename t_p>
struct canonical_type_f<t_p&>
{
    using type = typename canonical_type_f<t_p>::type;
};

// specialization for smart pointers
template <typename t_p>
struct canonical_type_f<std::shared_ptr<t_p>>
{
    using type = typename canonical_type_f<t_p>::type;
};

// base case: strip cv-qualifiers
template <typename t_p>
struct canonical_type_f
{
    using type = std::remove_cv_t<t_p>;
};

// helper alias
template <typename t_p>
using canonical_type_t = typename canonical_type_f<t_p>::type;

template <typename interface_t>
using default_binding_t
    = binding::binding_t<interface_t, binding::providers::type_t<interface_t>, binding::scopes::transient_t>;

/*
Containers parameterize on their config tuple, so each container has a unique type. The root container has no parent. Nested containers parameterize on their parent type and take a reference to an instance of one in their constructor. Part of resolving a request for an instance means trying to find a binding for the requested type, first locally, then in the parent, then continuing up the parent chain until one is found, or none is found. If none is found, the type is transitive and uses the default transient scope and simple ctor provider. Because the types are different for each level of nested container, walking up the chain can't use a while loop; it must call into the parent. When searching for a binding up the chain, what is returned? It is either an arbitarily-typed element of a tuple, or something signaling nothing was found, but how is this expressed?


The root container has a bindings tuple, a meyers singleton based cache for transitive instances, and cooks its config
to attach meyers singletons to scoped binding elements. It has no parent. 



*/

template <typename bindings_tuple_t, typename instance_cache_t = instance_cache_t>
class container_t
{
public:
    template <typename requested_t>
    auto resolve() -> requested_t
    {
        using unqualified_t = std::remove_cvref_t<requested_t>;
        using key_t = key_t<requested_t>;

        // see if type is bound or is a transitive dependency
        using bindings_key_type_list_t = get_key_types_t<bindings_tuple_t>;
        static constexpr auto bindings_index = type_list::index_of_v<bindings_key_type_list_t, key_t>;
        if constexpr (type_list::npos == bindings_index)
        {
            // no binding was found; this type is a transitive dependency discovered while walking the dependency tree
            return resolve_transitive_dependency<requested_t, unqualified_t, key_t>();
        }

        // this type has an explicit binding; use that
        return resolve_bound<requested_t, unqualified_t, key_t>(std::get<bindings_index>(bindings_tuple_));
    }

    container_t(bindings_tuple_t bindings_tuple, instance_cache_t instance_cache) noexcept
        : bindings_tuple_{std::move(bindings_tuple)}, instance_cache_{std::move(instance_cache)}
    {}

private:
    template <typename requested_t, typename unqualified_t, typename key_t, typename binding_t>
    auto resolve_transient(binding_t&& binding) -> requested_t
    {
        // just create it
    }

    template <typename requested_t, typename unqualified_t, typename key_t, typename binding_t>
    auto resolve_scoped(binding_t&& binding) -> requested_t
    {
        // find cache in correct place (binding if scoped, intance cache otherwise)
        static constexpr auto use_binding_cache
            = std::is_same_v<binding::scopes::scoped_t, typename binding_t::scope_t>;
        if constexpr (use_binding_cache)
        {
            return resolve_scoped_via_binding_cache<requested_t, unqualified_t, key_t, binding_t>(
                std::forward<binding_t>(binding)
            );
        }
        else
        {
            return resolve_scoped_via_instance_cache<requested_t, unqualified_t, key_t, binding_t>(
                std::forward<binding_t>(binding)
            );
        }
    }

    template <typename requested_t, typename unqualified_t, typename key_t, typename binding_t>
    auto resolve_singleton(binding_t&& binding) -> requested_t
    {}

    //! resolves a transitive type, one discovered as a dependency without an explicit binding
    template <typename requested_t, typename unqualified_t, typename key_t>
    auto resolve_transitive_dependency() -> requested_t
    {
        using default_binding_t = default_binding_t<key_t>;
        return resolve<requested_t, unqualified_t, key_t>(default_binding_t{});
    }

    template <typename requested_t, typename unqualified_t, typename key_t, typename binding_t>
    auto resolve_bound(binding_t&& binding) -> requested_t
    {
        using scope_t = binding_t::scope_t;

        static constexpr auto override_transient = !std::is_same_v<scope_t, binding::scopes::transient_t>
            && (std::is_rvalue_reference_v<requested_t> || is_unique_ptr_v<unqualified_t>);
        if constexpr (override_transient)
        {
            return resolve_transient<requested_t, unqualified_t, key_t>(std::forward<binding_t>(binding));
        }

        static constexpr auto definitely_scoped = !std::is_same_v<scope_t, binding::scopes::scoped_t>
            && (std::is_lvalue_reference_v<requested_t>
                || (std::is_pointer_v<requested_t> && !(std::is_function_v<std::remove_pointer_t<unqualified_t>>))
                || is_weak_ptr_v<unqualified_t>);
        if constexpr (definitely_scoped)
        {
            return resolve_scoped<requested_t, unqualified_t, key_t>(std::forward<binding_t>(binding));
        }

        if constexpr (std::is_same_v<scope_t, binding::scopes::transient_t>)
        {
            return resolve_transient<requested_t, unqualified_t, key_t>(std::forward<binding_t>(binding));
        }

        if constexpr (std::is_same_v<scope_t, binding::scopes::scoped_t>)
        {
            return resolve_scoped<requested_t, unqualified_t, key_t>(std::forward<binding_t>(binding));
        }

        return resolve_singleton<requested_t, unqualified_t, key_t>(std::forward<binding_t>(binding));
    }

    template <typename smart_ptr_t, typename canonical_t>
    auto create_new_instance() -> smart_ptr_t
    {
        auto& provider = container_->template provider<canonical_t>();
        return smart_ptr_t{new canonical_t{provider.get(*this)}};
    }

    template <typename canonical_t>
    auto get_shared_instance() -> std::shared_ptr<canonical_t>{
        // auto cache_entry = container_->template
        // check cache for T...
        // if miss:
        //   call provider to create T by value...
        //   `new T(std::move(value))` and store in cache as shared_ptr...
        // return the shared_ptr from the cache...
    }

    bindings_tuple_t bindings_tuple_;
    instance_cache_t instance_cache_;
};

} // namespace dink
