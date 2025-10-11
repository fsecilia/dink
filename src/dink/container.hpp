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
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

template <typename scope_t, typename config_t>
class container_t;

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename config_t>
struct is_container_f<container_t<scope_t, config_t>> : std::true_type
{};

template <typename T> concept is_container = is_container_f<std::decay_t<T>>::value;

template <typename scope_t, typename config_t>
class container_t
{
public:
    //! default ctor; produces container with global scope and no bindings
    container_t() = default;

    //! global scope ctor; produces container with global scope and given bindings
    template <typename first_binding_t, typename... remaining_bindings_t>
    requires(
        !is_container<std::decay_t<first_binding_t>>
        && is_binding<std::decay_t<first_binding_t>>
        && (is_binding<std::decay_t<remaining_bindings_t>> && ...)
    )
    explicit container_t(first_binding_t&& first, remaining_bindings_t&&... rest)
        : container_t{
              scope_t{},
              config_t{
                  resolve_bindings(std::forward<first_binding_t>(first), std::forward<remaining_bindings_t>(rest)...)
              }
          }
    {}

    //! nested scope ctor; produces container with nested scope and given bindings
    template <typename scope_p, typename config_p, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<scope_p, config_p>& parent, bindings_t&&... configs)
        : container_t{scope_t{parent}, config_t{resolve_bindings(std::forward<bindings_t>(configs)...)}}
    {}

    // basic ctor; must be constrained or it is too greedy for clang when using deduction guides
    template <is_scope scope_p, is_config config_p>
    container_t(scope_p&& scope, config_p&& config) noexcept
        : scope_{std::forward<scope_p>(scope)}, config_{std::forward<config_p>(config)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> decltype(auto)
    {
        using traits = request_traits_f<request_t>;
        using resolved_t = typename traits::value_type;

        static constexpr auto check_cache = traits::transitive_lifestyle == transitive_lifestyle_t::singleton;
        if constexpr (check_cache)
        {
            static constexpr auto check_shared_cache = is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>;
            if constexpr (check_shared_cache)
            {
                if (auto cached = scope_.template find_shared<resolved_t>())
                {
                    return as_requested<request_t>(std::move(cached));
                }
            }
            else
            {
                if (auto cached = scope_.template find<resolved_t>())
                {
                    return as_requested<request_t>(std::move(cached));
                }
            }
        }

        // check local bindings
        auto local_binding = config_.template find_binding<resolved_t>();
        static constexpr auto binding_found = !std::is_same_v<decltype(local_binding), not_found_t>;
        if constexpr (binding_found) return create_from_binding<request_t, dependency_chain_t>(*local_binding);

        // try delegating to parent
        decltype(auto) delegate_result = scope_.template delegate_to_parent<request_t, dependency_chain_t>();
        static constexpr auto delegate_succeeded = !std::is_same_v<decltype(delegate_result), not_found_t>;
        if constexpr (delegate_succeeded) return as_requested<request_t>(delegate_result);

        // no cached instances or bindings were found; create and optionally cache using default provider
        return create_from_default_provider<request_t, dependency_chain_t>();
    }

private:
    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t>
    auto create_from_binding(binding_t& binding) -> returned_t<request_t>
    {
        if constexpr (providers::is_accessor<typename binding_t::provider_type>)
        {
            // accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // creator - check effective lifestyle
            using binding_lifestyle_t = typename binding_t::lifestyle_type;
            using effective_lifestyle_t = effective_lifestyle_t<binding_lifestyle_t, request_t>;
            return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(binding.provider);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t>
    auto create_from_default_provider() -> returned_t<request_t>
    {
        using effective_lifestyle_t = effective_lifestyle_t<lifestyle::transient_t, request_t>;
        auto provider = providers::default_t<request_t>{};
        return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(provider);
    }

    template <typename request_t, typename dependency_chain_t, typename lifestyle_t, typename provider_t>
    auto execute_provider(provider_t& provider) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;
        using resolved_t = resolved_t<request_t>; // The underlying T

        if constexpr (std::same_as<lifestyle_t, lifestyle::singleton_t>)
        {
            // --- SINGLETON LIFESTYLE LOGIC ---

            // This is the core fix: Handle unique_ptr as a special case for singletons.
            // The user wants a unique copy of the canonical singleton instance.
            if constexpr (is_unique_ptr_v<request_t>) // Assume is_unique_ptr_v exists
            {
                // 1. Resolve the singleton instance. This will create and cache it on the first call,
                //    and return a reference to the cached instance on subsequent calls.
                auto& singleton_instance = scope_.template resolve<provided_t, dependency_chain_t>(provider, *this);

                // 2. Create a copy of the singleton and return it in a unique_ptr.
                return std::make_unique<resolved_t>(singleton_instance);
            }
            else if constexpr (is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>)
            {
                // Use the scope's dedicated shared_ptr handling, which correctly
                // creates a non-owning or cached shared_ptr to the singleton.
                return as_requested<request_t>(
                    scope_.template resolve_shared<provided_t, dependency_chain_t>(provider, *this)
                );
            }
            else
            {
                // For other requests (T&, T*), resolve the singleton and return a reference to it.
                return as_requested<request_t>(
                    scope_.template resolve<provided_t, dependency_chain_t>(provider, *this)
                );
            }
        }
        else // --- TRANSIENT LIFESTYLE LOGIC ---
        {
            // Create a new instance every time, without caching.
            return as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
        }
    }

    scope_t scope_{};
    config_t config_{};
};

/*
    deduction guides

    clang matches the global deduction guide for a nested container, so there are clang-20.1-specific workarounds:
        - the global deduction guides must be split into empty and nonempty so we can apply a constraint to the first
          parameter
        - the nonempty version must use enable_if_t to remove itself from consideration

    When clang fixes this, the empty/nonempty split can be removed, though something like the enable_if_t will need to 
    be converted to a concept.
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
    scope::global_t,
    typename config_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type>;

//! deduction guide for empty global containers
container_t() -> container_t<scope::global_t, config_t<>>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_config_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_config_t>& parent, bindings_t&&...) -> container_t<
    scope::nested_t<std::decay_t<decltype(parent)>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    scope::global_t, typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    scope::nested_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

} // namespace dink
