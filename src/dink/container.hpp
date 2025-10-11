/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/config.hpp>
#include <dink/lifestyle.hpp>
#include <dink/not_found.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <dink/resolver.hpp>
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

template <typename scope_t, typename config_t, typename resolver_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename config_t, typename resolver_t>
struct is_container_f<container_t<scope_t, config_t, resolver_t>> : std::true_type
{};

// In a detail namespace
template <typename>
struct is_reference_wrapper : std::false_type
{};

template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{};

template <typename T>
inline constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

} // namespace detail

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

template <typename scope_t, typename config_t, typename resolver_t>
class container_t : public scope_t
{
    // Helper to get the deduced config type from a set of configs
    template <typename... bindings_t>
    using deduced_config_t =
        typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type;

public:
    container_t(scope_t scope, config_t config, resolver_t resolver) noexcept
        : scope_t{std::move(scope)}, config_{std::move(config)}, resolver_{std::move(resolver)}
    {}

    // global constructor
    template <typename... bindings_t>
    requires((!is_container<bindings_t> && ...) && (is_binding<bindings_t> && ...))
    explicit container_t(bindings_t&&... configs) : config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    // nested constructor
    template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&... configs)
        : scope_t{parent}, config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        // 1. Check local cache (Runtime check)
        // This is the highest precedence lookup. If it hits, we're done.
        if (auto cached = scope_t::template find_in_local_cache<resolved_t>(); cached)
        {
            using cached_t = decltype(cached);
            if constexpr (std::is_pointer_v<cached_t>) // True for global_t, which returns T*
            {
                return *cached; // Return the reference to the singleton
            }
            else // True for nested_t, which returns shared_ptr<T>
            {
                return as_requested<request_t>(std::move(cached));
            }
        }

        // --- If we get here, the cache missed ---

        // 2. Check local bindings (Compile-time check)
        auto binding = config_.template find_binding<resolved_t>();
        if constexpr (!std::is_same_v<decltype(binding), not_found_t>)
        {
            // A binding was found. Create from it and we're done.
            return resolver_.template create_from_binding<request_t, dependency_chain_t>(*binding, *this);
        }

        // --- If we get here, cache missed and no local binding exists ---

        // 3. Delegate or use default provider (Compile-time check)
        static constexpr auto is_global = std::same_as<scope_t, container::scope::global_t>;
        if constexpr (!is_global)
        {
            // Not the global scope, so delegate the entire resolution task to the parent.
            return scope_t::parent->template resolve<request_t, dependency_chain_t>();
        }
        else
        {
            // We are the global scope (the end of the line). If not found here, use the default.
            return resolver_.template create_from_default_provider<request_t, dependency_chain_t>(*this);
        }
    }

private:
    config_t config_{};
    resolver_t resolver_{};
};

/*
    deduction guides
    
    clang kept matching the global deduction guide for a nested container, so there are 2 clang-20.1-specific workarounds:
        - the global deduction guides must be split into empty and nonempty so we can apply a constrainted to the first_
          parameter
        - the nonempty version must use enable_if_t to remove itself from consideration
    
    When clang fixes this, the empty/nonempty split can be removed, as can the enable_if_t.
*/

//! deduction guide for nonempty global containers
template <
    typename first_binding_p, typename... rest_bindings_p,
    std::enable_if_t<
        !is_container<std::decay_t<first_binding_p>>
            && is_binding<std::decay_t<first_binding_p>>
            && (is_binding<std::decay_t<rest_bindings_p>> && ...),
        int>
    = 0>
container_t(first_binding_p&&, rest_bindings_p&&...) -> container_t<
    container::scope::global_t,
    typename config_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type,
    resolver_t>;

//! deduction guide for empty global containers
container_t() -> container_t<container::scope::global_t, config_t<>, resolver_t>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&...) -> container_t<
    container::scope::nested_t<std::decay_t<decltype(parent)>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    container::scope::global_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    container::scope::nested_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, dink::resolver_t>;

} // namespace dink
