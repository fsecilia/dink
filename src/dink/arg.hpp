/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/meta.hpp>
#include <dink/type_list.hpp>
#include <concepts>
#include <utility>

namespace dink {

//! matches any argument type to produce an instance from a container; not fit to match single-argument ctors
template <typename container_t, typename dependency_chain_t>
class arg_t
{
public:
    /*!
        value conversion
        
        This conversion matches everything but lvalue refs, including rvalue refs and pointers.
        
        This method is NOT const to break ties in overload resolution, even though it normally should be.
    */
    template <typename deduced_t>
    constexpr operator deduced_t()
    {
        return resolve<deduced_t, canonical_t<deduced_t>>();
    }

    /*!
        reference conversion
        
        This conversion matches lvalue refs, so deduced_t& and deduced_t const&.
    */
    template <typename deduced_t>
    constexpr operator deduced_t&() const
    {
        return resolve<deduced_t&, canonical_t<deduced_t>>();
    }

    explicit constexpr arg_t(container_t& container) noexcept : container_{container} {}

private:
    container_t& container_;

    template <typename canonical_deduced_t>
    static constexpr auto assert_noncircular() noexcept -> void
    {
        static_assert(
            meta::dependent_bool_v<!type_list::contains_v<dependency_chain_t, canonical_deduced_t>, dependency_chain_t>,
            "circular dependency detected"
        );
    }

    template <typename deduced_t, typename canonical_deduced_t>
    constexpr auto resolve() const -> deduced_t
    {
        assert_noncircular<canonical_deduced_t>();
        using next_dependency_chain_t = type_list::append_t<dependency_chain_t, canonical_deduced_t>;
        return container_.template resolve<deduced_t, next_dependency_chain_t>();
    }
};

// ---------------------------------------------------------------------------------------------------------------------

//! filters out signatures that match copy ctor or move ctor
template <typename deduced_t, typename resolved_t>
concept single_arg_deducible = !std::same_as<std::decay_t<deduced_t>, resolved_t>;

//! matches any argument type to produce an instance from a container; excludes signatures that match copy or move ctor
template <typename resolved_t, typename arg_t>
class single_arg_t
{
public:
    /*!
        value conversion

        deliberately not const for the same reason as in arg_t
        
        /sa arg_t<resolved_t, dependency_chain_t>::operator deduced_t()
    */
    template <single_arg_deducible<resolved_t> deduced_t>
    constexpr operator deduced_t()
    {
        return arg_.operator deduced_t();
    }

    /*!
        reference conversion

        /sa arg_t<resolved_t, dependency_chain_t>::operator deduced_t&()
    */
    template <single_arg_deducible<resolved_t> deduced_t>
    constexpr operator deduced_t&() const
    {
        return arg_.operator deduced_t&();
    }

    explicit constexpr single_arg_t(arg_t arg) noexcept : arg_{std::move(arg)} {}

private:
    arg_t arg_;
};

} // namespace dink
