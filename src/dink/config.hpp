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

//! finds bound scope for
template <std::size_t index, typename bindings_tuple_p>
struct bound_scope_f;

//! general case where binding is found at a valid index
template <std::size_t index, typename bindings_tuple_p>
struct bound_scope_f
{
    using binding_t = std::tuple_element_t<index, bindings_tuple_p>;
    using type = typename binding_t::scope_type;
};

//! specialization when the binding is not found
template <typename bindings_tuple_p>
struct bound_scope_f<npos, bindings_tuple_p>
{
    using type = scope::transient_t;
};

} // namespace detail

// Manages finding a binding for a given type
template <typename... bindings_t>
class config_t
{
public:
    explicit config_t(std::tuple<bindings_t...> bindings) : bindings_{std::move(bindings)} {}

    template <typename... args_t>
    explicit config_t(args_t&&... args) : bindings_{std::forward<args_t>(args)...}
    {}

    template <typename resolved_t>
    auto find_binding() -> auto
    {
        constexpr auto index = binding_index_v<resolved_t>;
        if constexpr (index != npos) return &std::get<index>(bindings_);
        else return not_found;
    }

private:
    // Compute binding index at compile time
    template <typename resolved_t>
    static constexpr std::size_t compute_binding_index()
    {
        if constexpr (sizeof...(bindings_t) == 0) return npos;
        else
        {
            return []<std::size_t... indices>(std::index_sequence<indices...>) consteval {
                // build array of matches
                constexpr bool matches[] = {std::same_as<
                    resolved_t, typename std::tuple_element_t<indices, std::tuple<bindings_t...>>::from_type>...};

                // find first match in array
                for (std::size_t index = 0; index < sizeof...(indices); ++index)
                {
                    if (matches[index]) return index;
                }

                // no match was found
                return npos;
            }(std::index_sequence_for<bindings_t...>{});
        }
    }

    template <typename resolved_t>
    static constexpr std::size_t binding_index_v = compute_binding_index<resolved_t>();

    using bindings_tuple_t = std::tuple<bindings_t...>;
    bindings_tuple_t bindings_;

public:
    // determines the scope type from binding index
    template <typename resolved_t>
    using bound_scope_t = typename detail::bound_scope_f<binding_index_v<resolved_t>, bindings_tuple_t>::type;
};

template <typename... bindings_t>
config_t(bindings_t&&...) -> config_t<std::remove_cvref_t<bindings_t>...>;

template <typename config_t>
concept is_config = requires(config_t& config) {
    config.template find_binding<meta::concept_probe_t>();
    typename config_t::template bound_scope_t<meta::concept_probe_t>;
};

/// \brief A metafunction to create a config type from a tuple of bindings.
template <typename tuple_t>
struct config_from_tuple_f;

/// \brief Specialization that extracts the binding pack from the tuple.
template <template <typename...> class tuple_p, typename... bindings_t>
struct config_from_tuple_f<tuple_p<bindings_t...>>
{
    using type = config_t<bindings_t...>;
};

} // namespace dink
