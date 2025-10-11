/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/not_found.hpp>
#include <tuple>
#include <utility>

namespace dink {

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
        if constexpr (index != static_cast<std::size_t>(-1)) return &std::get<index>(bindings_);
        else return not_found;
    }

private:
    // Compute binding index at compile time
    template <typename T>
    static constexpr std::size_t compute_binding_index()
    {
        if constexpr (sizeof...(bindings_t) == 0) { return static_cast<std::size_t>(-1); }
        else
        {
            return []<std::size_t... Is>(std::index_sequence<Is...>) consteval {
                constexpr std::size_t not_found = static_cast<std::size_t>(-1);

                // Build array of matches
                constexpr bool matches[]
                    = {std::same_as<T, typename std::tuple_element_t<Is, std::tuple<bindings_t...>>::from_type>...};

                // Find first match
                for (std::size_t i = 0; i < sizeof...(Is); ++i)
                {
                    if (matches[i]) return i;
                }
                return not_found;
            }(std::index_sequence_for<bindings_t...>{});
        }
    }

    template <typename T>
    static constexpr std::size_t binding_index_v = compute_binding_index<T>();

    std::tuple<bindings_t...> bindings_;
};

template <typename... bindings_t>
config_t(bindings_t&&...) -> config_t<std::remove_cvref_t<bindings_t>...>;

/// \brief A metafunction to create a config type from a tuple of bindings.
template <typename tuple_t>
struct config_from_tuple_f;

/// \brief Specialization that extracts the binding pack from the tuple.
template <template <typename...> class tuple_p, typename... bindings_t>
struct config_from_tuple_f<tuple_p<bindings_t...>>
{
    using type = config_t<bindings_t...>;
};

template <typename>
struct is_config_f : std::false_type
{};

template <typename... Bindings>
struct is_config_f<config_t<Bindings...>> : std::true_type
{};

template <typename T> concept is_config = is_config_f<std::remove_cvref_t<T>>::value;

} // namespace dink
