/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace dink::resolvers {

//! transient requests are dispatched to a factory and returned directly
template <
    template <typename, typename, typename, template <typename, typename, int_t> class, typename...> class dispatcher_t,
    template <typename> class factory_t, template <typename> class bindings_t,
    template <typename, typename, int_t> class arg_t>
class transient_t
{
public:
    template <typename resolved_t, typename composer_t>
    constexpr auto resolve(composer_t& composer) -> resolved_t
    {
        if constexpr (std::copy_constructible<resolved_t>)
        {
            auto& binding = bindings_<resolved_t>;
            if (binding.is_bound()) return binding.bound();
        }
        using resolved_dispatcher_t = dispatcher_t<resolved_t, composer_t, factory_t<resolved_t>, arg_t>;
        static constexpr auto dispatch_successful = resolved_dispatcher_t::resolved;
        static_assert(dispatch_successful);
        return resolved_dispatcher_t{}(composer);
    }

    template <typename resolved_t>
    constexpr auto bind(resolved_t resolved) -> void
    {
        bindings_<resolved_t>.bind(std::move(resolved));
    }

    template <typename resolved_t>
    constexpr auto unbind() noexcept -> void
    {
        bindings_<resolved_t>.unbind();
    }

    template <typename resolved_t>
    constexpr auto is_bound() const noexcept -> bool
    {
        return bindings_<resolved_t>.is_bound();
    }

    template <typename resolved_t>
    constexpr auto bound() const noexcept -> resolved_t&
    {
        return bindings_<resolved_t>.bound();
    }

private:
    template <typename resolved_t>
    inline static bindings_t<resolved_t> bindings_{};
};

//! shared requests remove refness, resolve via the composer, store the result in a scope, then return scoped
template <template <typename> class bindings_t, typename scope_tp, template <typename> class nested_scope_tp>
class shared_t
{
public:
    using scope_t = scope_tp;

    template <typename resolved_t, typename composer_t>
    constexpr auto resolve(composer_t& composer) -> resolved_t&
    {
        using canonical_resolved_t = canonical_t<resolved_t>;
        auto& binding = bindings_<canonical_resolved_t>;
        if (binding.is_bound()) return binding.template bound<resolved_t>();
        return scope_.template resolve<canonical_resolved_t>(composer);
    }

    template <typename resolved_t>
    constexpr auto bind(resolved_t&& resolved) -> void
    {
        bindings_<canonical_t<resolved_t>>.bind(std::forward<resolved_t>(resolved));
    }

    template <typename resolved_t>
    constexpr auto unbind() noexcept -> void
    {
        bindings_<canonical_t<resolved_t>>.unbind();
    }

    template <typename resolved_t>
    constexpr auto is_bound() const noexcept -> bool
    {
        return bindings_<canonical_t<resolved_t>>.is_bound();
    }

    template <typename resolved_t>
    constexpr auto bound() const noexcept -> resolved_t&
    {
        return bindings_<canonical_t<resolved_t>>.bound();
    }

    constexpr auto scope() const noexcept -> scope_t const& { return scope_; }
    constexpr auto scope() noexcept -> scope_t& { return scope_; }

    using nested_t = shared_t<bindings_t, nested_scope_tp<scope_t>, nested_scope_tp>;
    constexpr auto create_nested() -> nested_t
    {
        using nested_scope_t = nested_t::scope_t;
        return nested_t{nested_scope_t{scope_}};
    }

    explicit constexpr shared_t(scope_t scope) : scope_{std::move(scope)} {}

private:
    template <typename resolved_t>
    using canonical_t = std::remove_cvref_t<resolved_t>;

    template <typename resolved_t>
    inline static bindings_t<resolved_t> bindings_{};

    scope_t scope_;
};

} // namespace dink::resolvers
