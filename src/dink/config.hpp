/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/meta.hpp>
#include <dink/not_found.hpp>
#include <tuple>
#include <utility>

namespace dink {
namespace detail {

//! finds binding index for resolved_t in bindings_tuple_t
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
struct binding_index_f;

template <typename resolved_t, typename bindings_tuple_t>
inline static constexpr std::size_t binding_index_v = binding_index_f<resolved_t, 0, bindings_tuple_t>::value;

// ---------------------------------------------------------------------------------------------------------------------

//! finds bound scope for given index, or default for npos.
template <std::size_t index, typename bindings_tuple_t>
struct bound_scope_f;

template <std::size_t index, typename bindings_tuple_t>
using bound_scope_t = typename bound_scope_f<index, bindings_tuple_t>::type;

} // namespace detail

//! bindings container; allows searching for bindings by type and bound scope by type
template <typename... bindings_t>
class config_t
{
    using bindings_tuple_t = std::tuple<bindings_t...>;
    bindings_tuple_t bindings_;

public:
    explicit config_t(std::tuple<bindings_t...> bindings) : bindings_{std::move(bindings)} {}

    template <typename... args_t>
    explicit config_t(args_t&&... args) : bindings_{std::forward<args_t>(args)...}
    {}

    template <typename resolved_t>
    auto find_binding() -> auto
    {
        static constexpr auto index = detail::binding_index_v<resolved_t, bindings_tuple_t>;
        if constexpr (index != npos) return &std::get<index>(bindings_);
        else return not_found;
    }

    // determines the scope type from binding index
    template <typename resolved_t>
    using bound_scope_t
        = detail::bound_scope_t<detail::binding_index_v<resolved_t, bindings_tuple_t>, bindings_tuple_t>;
};

template <typename... bindings_t>
config_t(bindings_t&&...) -> config_t<std::remove_cvref_t<bindings_t>...>;

template <typename config_t>
concept is_config = requires(config_t& config) {
    config.template find_binding<meta::concept_probe_t>();
    typename config_t::template bound_scope_t<meta::concept_probe_t>;
};

namespace detail {

//! base case: not found
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
struct binding_index_f
{
    static constexpr std::size_t value = npos;
};

// recursive case: check current binding
template <typename resolved_t, std::size_t index, typename bindings_tuple_t>
requires(index < std::tuple_size_v<bindings_tuple_t>)
struct binding_index_f<resolved_t, index, bindings_tuple_t>
{
    using current_binding_t = std::tuple_element_t<index, bindings_tuple_t>;

    static constexpr std::size_t value = std::same_as<resolved_t, typename current_binding_t::from_type>
        ? index
        : binding_index_f<resolved_t, index + 1, bindings_tuple_t>::value;
};

// ---------------------------------------------------------------------------------------------------------------------

//! general case where binding is found at a valid index
template <std::size_t index, typename bindings_tuple_t>
struct bound_scope_f
{
    using binding_t = std::tuple_element_t<index, bindings_tuple_t>;
    using type = typename binding_t::scope_type;
};

//! specialization when the binding is not found
template <typename bindings_tuple_t>
struct bound_scope_f<npos, bindings_tuple_t>
{
    using type = scope::transient_t;
};

// ---------------------------------------------------------------------------------------------------------------------

//! metafunction to create a config type from a tuple of bindings
template <typename tuple_t>
struct config_from_tuple_f;

//! specialization to extract binding pack from tuple
template <template <typename...> class tuple_t, typename... bindings_t>
struct config_from_tuple_f<tuple_t<bindings_t...>>
{
    using type = config_t<bindings_t...>;
};

//! creates a config from a tuple of bindings
template <typename tuple_t>
using config_from_tuple_t = typename config_from_tuple_f<tuple_t>::type;

} // namespace detail
} // namespace dink
