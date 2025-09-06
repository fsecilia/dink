/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

namespace dink {

template <typename tuple_t>
struct flatten_tuple_f;

template <typename tuple_t>
using flatten_tuple_t = typename flatten_tuple_f<std::remove_cvref_t<tuple_t>>::type;

template <typename tuple_t>
constexpr auto flatten_tuple(tuple_t&& tuple) -> flatten_tuple_t<tuple_t>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename element_t>
struct flatten_tuple_f
{
    using type = std::tuple<element_t>;

    template <typename immediate_element_t>
    constexpr auto operator()(immediate_element_t&& element) const -> type
    {
        return type{std::forward<immediate_element_t>(element)};
    }
};

template <typename... elements_t>
struct flatten_tuple_f<std::tuple<elements_t...>>
{
    using type = decltype(std::tuple_cat(std::declval<typename flatten_tuple_f<elements_t>::type>()...));

    template <typename immediate_tuple_t>
    constexpr auto operator()(immediate_tuple_t&& tuple) const -> type
    {
        return std::apply(
            [](auto&&... elements) {
                return std::tuple_cat(flatten_tuple(std::forward<decltype(elements)>(elements))...);
            },
            std::forward<immediate_tuple_t>(tuple)
        );
    }
};

template <typename tuple_t>
constexpr auto flatten_tuple(tuple_t&& tuple) -> flatten_tuple_t<tuple_t>
{
    constexpr auto impl = flatten_tuple_f<std::remove_cvref_t<tuple_t>>{};
    return impl(std::forward<tuple_t>(tuple));
}

// ---------------------------------------------------------------------------------------------------------------------

template <typename... elements_t>
using flatten_t = flatten_tuple_t<std::tuple<std::remove_cvref_t<elements_t>...>>;

template <typename... elements_t>
constexpr auto flatten(elements_t&&... elements) -> flatten_t<elements_t...>
{
    return flatten_tuple(std::make_tuple(std::forward<elements_t>(elements)...));
}

} // namespace dink
