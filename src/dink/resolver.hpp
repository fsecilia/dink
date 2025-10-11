/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/lifestyle.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <memory>

namespace dink {

// Creates an instance from a provider, handling lifestyle and caching.
class resolver_t
{
public:
    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t, typename container_p>
    auto create_from_binding(binding_t& binding, container_p& container) -> returned_t<request_t>
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
            return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(binding.provider, container);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t, typename container_p>
    auto create_from_default_provider(container_p& container) -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        providers::creator_t<resolved_t> default_provider;
        using effective_lifestyle_t = effective_lifestyle_t<lifestyle::transient_t, request_t>;
        return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(default_provider, container);
    }

private:
    template <
        typename request_t, typename dependency_chain_t, typename lifestyle_t, typename provider_t,
        typename container_p>
    auto execute_provider(provider_t& provider, container_p& container) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;

        // --- BRANCH ON REQUEST TYPE ---
        if constexpr (detail::is_shared_ptr_v<request_t> || detail::is_weak_ptr_v<request_t>)
        {
            // --- LOGIC FOR SHARED POINTERS (P4, P5, P7) ---
            if constexpr (std::same_as<lifestyle_t, lifestyle::singleton_t>)
            {
                // This is a singleton request, so use the scope's shared_ptr container.
                // This correctly handles caching the canonical shared_ptr.
                return as_requested<request_t>(
                    container.template resolve_shared_ptr<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // transient lifestyle
            {
                // create a new transient shared_ptr.
                return as_requested<request_t>(
                    std::shared_ptr<provided_t>{new provided_t{provider.template create<dependency_chain_t>(container)}}
                );
            }
        }
        else
        {
            static_assert(std::same_as<lifestyle_t, effective_lifestyle_t<lifestyle_t, request_t>>);

            // --- EXISTING LOGIC FOR OTHER TYPES (P1, P2, P3, P6) ---

            if constexpr (std::same_as<lifestyle_t, lifestyle::singleton_t>)
            {
                // Resolve through scope (caches automatically)
                return as_requested<request_t>(
                    container.template resolve_singleton<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // It's a transient lifestyle
            {
                // Create without caching
                return as_requested<request_t>(provider.template create<dependency_chain_t>(container));
            }
        }
    }
};

} // namespace dink
