/*!
    \file
    Copyright (C) 2025 Frank Secilia

    configuration storage for dependency injection bindings

    This file provides the config_t class which stores and indexes bindings, allowing O(1) lookup of bindings and their
    scopes by type at compile time.
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/meta.hpp>
#include <dink/not_found.hpp>
#include <dink/scope.hpp>
#include <tuple>
#include <utility>

namespace dink {

//! forward declaration
template <typename... bindings_t>
class config_t;

// ---------------------------------------------------------------------------------------------------------------------
// concepts
// ---------------------------------------------------------------------------------------------------------------------

/*!
    concept for valid configuration types

    A config must support:
    - finding bindings by resolved type
    - querying bound scope for a type
*/
template <typename config_t>
concept is_config = requires(config_t& config) {
    config.template find_binding<meta::concept_probe_t>();
    typename config_t::template bound_scope_t<meta::concept_probe_t>;
};

// ---------------------------------------------------------------------------------------------------------------------
// implementation details
// ---------------------------------------------------------------------------------------------------------------------

namespace detail {

/*!
    finds the index of a binding for resolved_t in the bindings tuple

    \tparam resolved_t type to search for
    \tparam index current search index
    \tparam bindings_tuple_t tuple of binding types
    \return index of binding, or npos if not found
*/
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
struct binding_index_f;

//! base case: value is not found
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
struct binding_index_f
{
    static constexpr auto value = npos;
};

//! recursive case: check if current binding matches, otherwise continue
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
requires(index < std::tuple_size_v<bindings_tuple_t>)
struct binding_index_f<resolved_t, index, bindings_tuple_t>
{
    using current_binding_t = std::tuple_element_t<index, bindings_tuple_t>;

    static constexpr auto value = std::same_as<resolved_t, typename current_binding_t::from_type>
        ? index
        : binding_index_f<resolved_t, index + 1, bindings_tuple_t>::value;
};

//! alias for finding binding index
template <typename resolved_t, typename bindings_tuple_t>
inline static constexpr auto binding_index_v = binding_index_f<resolved_t, 0, bindings_tuple_t>::value;

// ---------------------------------------------------------------------------------------------------------------------

/*!
    extracts the scope type from a binding at the given index

    \tparam index index of binding in tuple
    \tparam bindings_tuple_t tuple of binding types
*/
template <std::size_t index, typename bindings_tuple_t>
struct bound_scope_f;

//! general case - binding is found at given index
template <std::size_t index, typename bindings_tuple_t>
struct bound_scope_f
{
    using binding_t = std::tuple_element_t<index, bindings_tuple_t>;
    using type = typename binding_t::scope_type;
};

//! specialization when binding is not found - defaults to transient scope
template <typename bindings_tuple_t>
struct bound_scope_f<npos, bindings_tuple_t>
{
    using type = scope::transient_t;
};

//! alias for extracting bound scope
template <std::size_t index, typename bindings_tuple_t>
using bound_scope_t = typename bound_scope_f<index, bindings_tuple_t>::type;

// ---------------------------------------------------------------------------------------------------------------------

/*!
    creates a config_t type from a tuple of bindings

    Used primarily with deduction guides to convert tuple<bindings...> to config_t<bindings...>
*/
template <typename tuple_t>
struct config_from_tuple_f;

//! specialization to extract bindings from given tuple
template <template <typename...> class tuple_t, typename... bindings_t>
struct config_from_tuple_f<tuple_t<bindings_t...>>
{
    using type = config_t<bindings_t...>;
};

//! alias for creating config from tuple
template <typename tuple_t>
using config_from_tuple_t = typename config_from_tuple_f<tuple_t>::type;

} // namespace detail

// ---------------------------------------------------------------------------------------------------------------------
// configuration class
// ---------------------------------------------------------------------------------------------------------------------

/*!
    type-safe configuration storage for dependency injection bindings

    Stores bindings in a compile-time indexed tuple, enabling O(1) lookup by type.
    Each binding maps from a requested type (from_type) to:
    - The type to construct (to_type)
    - How to construct it (provider)
    - When to cache it (scope)

    \tparam bindings_t pack of binding_t types
*/
template <typename... bindings_t>
class config_t
{
public:
    using bindings_tuple_t = std::tuple<bindings_t...>;

    // -----------------------------------------------------------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------------------------------------------------------

    //! construct from a tuple of bindings
    explicit config_t(std::tuple<bindings_t...> bindings) : bindings_{std::move(bindings)} {}

    //! construct from individual binding arguments
    template <typename... args_t>
    explicit config_t(args_t&&... args) : bindings_{std::forward<args_t>(args)...}
    {}

    // -----------------------------------------------------------------------------------------------------------------
    // binding lookup
    // -----------------------------------------------------------------------------------------------------------------

    /*!
        finds binding for a resolved type

        \tparam resolved_t canonical type to look up
        \return pointer to binding if found, otherwise not_found sentinel
    */
    template <typename resolved_t>
    auto find_binding() -> auto
    {
        static constexpr auto index = detail::binding_index_v<resolved_t, bindings_tuple_t>;

        if constexpr (index != npos) { return &std::get<index>(bindings_); }
        else { return not_found; }
    }

    // -----------------------------------------------------------------------------------------------------------------
    // scope lookup
    // -----------------------------------------------------------------------------------------------------------------

    /*!
        gets the bound scope for a resolved type

        If no binding exists, defaults to transient scope.

        \tparam resolved_t canonical type to query
        \return scope type (transient_t, singleton_t, etc.)
    */
    template <typename resolved_t>
    using bound_scope_t
        = detail::bound_scope_t<detail::binding_index_v<resolved_t, bindings_tuple_t>, bindings_tuple_t>;

private:
    bindings_tuple_t bindings_;
};

// ---------------------------------------------------------------------------------------------------------------------
// deduction guides
// ---------------------------------------------------------------------------------------------------------------------

//! deduction guide to construct config from individual bindings
template <typename... bindings_t>
config_t(bindings_t&&...) -> config_t<std::remove_cvref_t<bindings_t>...>;

} // namespace dink
