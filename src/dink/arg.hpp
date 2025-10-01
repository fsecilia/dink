/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/type_list.hpp>
#include <dink/unqualified.hpp>
#include <concepts>
#include <utility>

namespace dink {

//! matches any argument type to produce an instance from a resolver; not fit to match single-argument ctors
template <typename resolver_t, typename dependency_chain_t>
class arg_t
{
public:
    /*!
        value-semantic conversion
        
        This conversion matches value-semantic types, like lvalues and rvalue refs. 
        
        This method is NOT const to break ties in overload resolution, even though it normally should be.
    */
    template <typename deduced_t>
    operator deduced_t()
    {
        assert_noncircular<deduced_t>();
        return resolver_
            .template resolve<deduced_t, type_list::append_t<dependency_chain_t, unqualified_t<deduced_t>>>();
    }

    /*!
        reference-semantic conversion
        
        This conversion matches reference-semantic types, like rvalues and pointers. 
    */
    template <typename deduced_t>
    operator deduced_t&() const
    {
        assert_noncircular<deduced_t>();
        return resolver_
            .template resolve<deduced_t&, type_list::append_t<dependency_chain_t, unqualified_t<deduced_t>>>();
    }

    explicit arg_t(resolver_t& resolver) noexcept : resolver_{resolver} {}

private:
    resolver_t& resolver_;

    template <typename deduced_t>
    static constexpr auto assert_noncircular() noexcept -> void
    {
        static_assert(
            meta::dependent_bool_v<!type_list::contains_v<dependency_chain_t, deduced_t>, dependency_chain_t>,
            "circular dependency detected"
        );
    }
};

// ---------------------------------------------------------------------------------------------------------------------

//! filters out signatures that match copy ctor or move ctor
template <typename deduced_t, typename resolved_t>
concept single_arg_deducible = !std::same_as<std::remove_cvref_t<deduced_t>, resolved_t>;

//! matches any argument type to produce an instance from a resolver; excludes signatures that match copy or move ctor
template <typename resolved_t, typename arg_t>
class single_arg_t
{
public:
    /*!
        value-semantic conversion

        deliberately not const for the same reason as in arg_t
        
        /sa arg_t<resolved_t, dependency_chain_t>::operator deduced_t()
    */
    template <single_arg_deducible<resolved_t> deduced_t>
    operator deduced_t()
    {
        return static_cast<deduced_t>(arg_);
    }

    /*!
        reference-semantic conversion

        /sa arg_t<resolved_t, dependency_chain_t>::operator deduced_t&()
    */
    template <single_arg_deducible<resolved_t> deduced_t>
    operator deduced_t&() const
    {
        return static_cast<deduced_t&>(arg_);
    }

    explicit single_arg_t(arg_t arg) noexcept : arg_{std::move(arg)} {}

private:
    arg_t arg_;
};

} // namespace dink
