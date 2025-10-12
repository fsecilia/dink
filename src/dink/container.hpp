/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/caching_policy.hpp>
#include <dink/config.hpp>
#include <dink/delegation_policy.hpp>
#include <dink/lifestyle.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/request_traits.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

template <typename policy_t>
concept is_container_policy = requires {
    typename policy_t::delegation_policy_t;
    typename policy_t::caching_policy_t;
    typename policy_t::default_provider_factory_t;
};

template <is_container_policy policy_t, is_config config_t>
class container_t;

template <typename>
struct is_container_f : std::false_type
{};

template <is_container_policy policy_t, is_config config_t>
struct is_container_f<container_t<policy_t, config_t>> : std::true_type
{};

template <typename value_t> concept is_container = is_container_f<std::remove_cvref_t<value_t>>::value;

template <is_container_policy policy_t, is_config config_t>
class container_t
{
public:
    using caching_policy_t = policy_t::caching_policy_t;
    using delegation_policy_t = policy_t::delegation_policy_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;

    //! default ctor; produces container with root container policy and no bindings
    container_t() = default;

    //! root container ctor; produces container with root container policy and given bindings
    template <typename first_binding_t, typename... remaining_bindings_t>
    requires(
        !is_container<std::remove_cvref_t<first_binding_t>>
        && is_binding<std::remove_cvref_t<first_binding_t>>
        && (is_binding<std::remove_cvref_t<remaining_bindings_t>> && ...)
    )
    explicit container_t(first_binding_t&& first, remaining_bindings_t&&... rest)
        : container_t{
              caching_policy_t{}, delegation_policy_t{},
              config_t{
                  resolve_bindings(std::forward<first_binding_t>(first), std::forward<remaining_bindings_t>(rest)...)
              },
              default_provider_factory_t{}
          }
    {}

    //! nested container ctor; produces container with nested container policy and given bindings
    template <typename parent_policy_t, typename parent_config_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<parent_policy_t, parent_config_t>& parent, bindings_t&&... configs)
        : container_t{
              caching_policy_t{}, delegation_policy_t{parent},
              config_t{resolve_bindings(std::forward<bindings_t>(configs)...)}, default_provider_factory_t{}
          }
    {}

    // fundamental ctor
    container_t(
        caching_policy_t caching_policy, delegation_policy_t delegation_policy, config_t config,
        default_provider_factory_t default_provider_factory
    ) noexcept
        : caching_policy_{std::move(caching_policy)}, delegation_policy_{std::move(delegation_policy)},
          config_{std::move(config)}, default_provider_factory_{std::move(default_provider_factory)}
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
                if (auto cached = caching_policy_.template find_shared<resolved_t>())
                {
                    return as_requested<request_t>(std::move(cached));
                }
            }
            else
            {
                if (auto cached = caching_policy_.template find<resolved_t>())
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
        decltype(auto) delegate_result = delegation_policy_.template delegate<request_t, dependency_chain_t>();
        static constexpr auto delegate_succeeded = !std::is_same_v<decltype(delegate_result), not_found_t>;
        if constexpr (delegate_succeeded) return as_requested<request_t>(delegate_result);

        // no cached instances or bindings were found; create and optionally cache using default provider
        return invoke_default_provider<request_t, dependency_chain_t>();
    }

private:
    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t>
    auto create_from_binding(binding_t& binding) -> returned_t<request_t>
    {
        if constexpr (provider::is_accessor<typename binding_t::provider_type>)
        {
            // accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // creator - check effective lifestyle
            using binding_lifestyle_t = typename binding_t::lifestyle_type;
            using effective_lifestyle_t = effective_lifestyle_t<binding_lifestyle_t, request_t>;
            return invoke_provider<request_t, dependency_chain_t, effective_lifestyle_t>(binding.provider);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t>
    auto invoke_default_provider() -> returned_t<request_t>
    {
        using effective_lifestyle_t = effective_lifestyle_t<lifestyle::transient_t, request_t>;
        auto default_provider = default_provider_factory_.template create<request_t>();
        return invoke_provider<request_t, dependency_chain_t, effective_lifestyle_t>(default_provider);
    }

    template <typename request_t, typename dependency_chain_t, typename lifestyle_t, typename provider_t>
    auto invoke_provider(provider_t& provider) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;

        if constexpr (std::same_as<lifestyle_t, lifestyle::singleton_t>)
        {
            if constexpr (is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>)
            {
                // caching_policy resolves a non-owning or cached shared_ptr to the singleton
                return as_requested<request_t>(
                    caching_policy_.template resolve_shared<provided_t, dependency_chain_t>(provider, *this)
                );
            }
            else
            {
                // caching_policy resolves a reference to the singleton
                return as_requested<request_t>(
                    caching_policy_.template resolve<provided_t, dependency_chain_t>(provider, *this)
                );
            }
        }
        else // --- TRANSIENT LIFESTYLE LOGIC ---
        {
            // Create a new instance every time, without caching.
            return as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
        }
    }

    caching_policy_t caching_policy_{};
    delegation_policy_t delegation_policy_;
    config_t config_{};
    [[no_unique_address]] default_provider_factory_t default_provider_factory_{};
};

// named policies

//! policy used to construct a root container
struct root_container_policy_t
{
    using delegation_policy_t = delegation_policy::root_t;
    using caching_policy_t = caching_policy::type_indexed_t<>;
    using default_provider_factory_t = provider::default_factory_t;
};

//! policy used to construct a nested container
template <typename parent_t>
struct nested_container_policy_t
{
    using delegation_policy_t = delegation_policy::nested_t<parent_t>;
    using caching_policy_t = caching_policy::hash_table_t<>;
    using default_provider_factory_t = provider::default_factory_t;
};

/*
    deduction guides

    clang matches the root deduction guide for a nested container, so there are clang-20.1-specific workarounds:
        - the root deduction guides must be split into empty and nonempty so we can apply a constraint to the first
          parameter
        - the nonempty version must use enable_if_t to remove itself from consideration

    When clang fixes this, the empty/nonempty split can be removed, though something like the enable_if_t will need to 
    be converted to a concept.
*/

//! deduction guide for nonempty root containers
template <
    typename first_binding_p, typename... rest_bindings_p,
    std::enable_if_t<
        !is_container<std::remove_cvref_t<first_binding_p>>
            && is_binding<std::remove_cvref_t<first_binding_p>>
            && (is_binding<std::remove_cvref_t<rest_bindings_p>> && ...),
        int>
    = 0>
container_t(first_binding_p&&, rest_bindings_p&&...) -> container_t<
    root_container_policy_t,
    typename config_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type>;

//! deduction guide for empty root containers
container_t() -> container_t<root_container_policy_t, config_t<>>;

//! deduction guide for nested containers
template <typename parent_container_policy_t, typename parent_config_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<parent_container_policy_t, parent_config_t>& parent, bindings_t&&...) -> container_t<
    nested_container_policy_t<container_t<parent_container_policy_t, parent_config_t>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

// type aliases

template <typename... bindings_t>
using root_container_t = container_t<
    root_container_policy_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

template <typename parent_container_policy_t, typename parent_config_t, typename... bindings_t>
using nested_container_t = container_t<
    nested_container_policy_t<container_t<parent_container_policy_t, parent_config_t>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

} // namespace dink
