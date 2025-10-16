/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arg.hpp>
#include <dink/ctor_factory.hpp>
#include <dink/meta.hpp>
#include <dink/not_found.hpp>
#include <utility>

namespace dink {

namespace arity {

namespace detail {

//! lightweight version of \ref `arg_t` used for probing arity
struct probe_arg_t {
    template <typename deduced_t>
    operator deduced_t();

    template <typename deduced_t>
    operator deduced_t&() const;
};

//! lightweight version of \ref `single_arg_t` used for probing arity
template <typename constructed_t>
struct single_probe_arg_t {
    template <single_arg_deducible<constructed_t> deduced_t>
    operator deduced_t();

    template <single_arg_deducible<constructed_t> deduced_t>
    operator deduced_t&() const;
};

//! `probe_arg_t`, for repetition by an index sequence
template <std::size_t index>
using args_t = meta::indexed_type_t<probe_arg_t, index>;

/*!
    investigates factory_t's call operators, looking for an overload with an arity based on the length of
    `index_sequence_t` that returns a type convertible to constructed_t, then checks the next smaller, all the way to 0
*/
template <typename constructed_t, typename factory_t, typename index_sequence_t>
struct arity_f;

//! general case checks current arity then advances to next smaller
template <typename constructed_t, typename factory_t, std::size_t index, std::size_t... remaining_indices>
struct arity_f<constructed_t, factory_t, std::index_sequence<index, remaining_indices...>> {
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        using next_arity_f = arity_f<constructed_t, factory_t, std::index_sequence<remaining_indices...>>;

        constexpr auto cur_arity = sizeof...(remaining_indices) + 1;
        using cur_arg_t          = std::conditional_t<cur_arity == 1, single_probe_arg_t<constructed_t>, probe_arg_t>;
        if constexpr (std::is_invocable_v<factory_t, cur_arg_t, args_t<remaining_indices>...>) {
            if constexpr (std::is_same_v<constructed_t,
                                         std::invoke_result_t<factory_t, cur_arg_t, args_t<remaining_indices>...>>) {
                return cur_arity;
            } else {
                return next_arity_f::value;
            }
        } else {
            return next_arity_f::value;
        }
    }();
};

//! base case checks 0-arity or resolves to npos
template <typename constructed_t, typename factory_t>
struct arity_f<constructed_t, factory_t, std::index_sequence<>> {
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        if constexpr (std::is_invocable_v<factory_t>) {
            if constexpr (std::is_convertible_v<constructed_t, std::invoke_result_t<factory_t>>) {
                return 0;
            } else {
                return npos;
            }
        } else {
            return npos;
        }
    }();
};

}  // namespace detail
}  // namespace arity

/*!
    searches `factory_t` to find arity of greediest call operator that returns a type convertible to `constructed_t`

    resolves to discovered arity or `npos`
*/
template <typename constructed_t, typename factory_t = ctor_factory_t<constructed_t>>
inline constexpr auto arity_v =
    arity::detail::arity_f<constructed_t, factory_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

}  // namespace dink
