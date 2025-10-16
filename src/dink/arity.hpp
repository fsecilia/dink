/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arg.hpp>
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
    resolves invoke_result_t only if is_invocable is already known to be true

    The expression std::is_invocable_v<factory_t, args_t...>
    && std::is_same<constructed_t, std::invoke_result_t<factory_t, args_t...>> is invalid if the factory is not
    invocable with those args. dependent_invoke_result_f produces the same logical expression, but only evaluates
    invoke_result_t after is_invocable_v is known to be true.
*/
template <bool is_invocable, typename constructed_t, typename factory_t, typename... args_t>
struct dependent_invoke_result_f : std::false_type {};

//! specialization dependent on is_invocable already being true
template <typename constructed_t, typename factory_t, typename... args_t>
struct dependent_invoke_result_f<true, constructed_t, factory_t, args_t...>
    : std::is_same<constructed_t, std::invoke_result_t<factory_t, args_t...>> {};

//! probes arity in factory call operators that return constructed_t, or ctors of constructed_t
template <typename constructed_t, typename factory_t, typename... args_t>
struct arity_probe_t;

//! generic parameterization matches a factory's call operator that returns an instance of constructed_t
template <typename constructed_t, typename factory_t, typename... args_t>
struct arity_probe_t {
    static constexpr auto matches = dependent_invoke_result_f<std::is_invocable_v<factory_t, args_t...>, constructed_t,
                                                              factory_t, args_t...>::value;
};

//! specialization for void factory matches constructed_t's ctor directly
template <typename constructed_t, typename... args_t>
struct arity_probe_t<constructed_t, void, args_t...> {
    static constexpr auto matches = std::is_constructible_v<constructed_t, args_t...>;
};

/*!
    probes arity of factory call operators or constructors to find the greediest overload

    This metafunction uses a generic arity probe to investigate either the call operators of a factory or the ctors of
    the constructed type. It looks for a matching overload with an arity based on the length of `index_sequence_t`,
    then checks the next smaller, all the way to 0.
*/
template <typename constructed_t, typename factory_t, typename index_sequence_t>
struct arity_f;

//! general case checks current arity then advances to next smaller
template <typename constructed_t, typename factory_t, std::size_t index, std::size_t... remaining_indices>
struct arity_f<constructed_t, factory_t, std::index_sequence<index, remaining_indices...>> {
    using next_arity_f = arity_f<constructed_t, factory_t, std::index_sequence<remaining_indices...>>;
    static constexpr std::size_t cur_arity = sizeof...(remaining_indices) + 1;
    using cur_arg_t = std::conditional_t<cur_arity == 1, single_probe_arg_t<constructed_t>, probe_arg_t>;

    static constexpr std::size_t value =
        arity_probe_t<constructed_t, factory_t, cur_arg_t, args_t<remaining_indices>...>::matches ? cur_arity
                                                                                                  : next_arity_f::value;
};

//! base case checks 0-arity or resolves to npos
template <typename constructed_t, typename factory_t>
struct arity_f<constructed_t, factory_t, std::index_sequence<>> {
    static constexpr std::size_t value = arity_probe_t<constructed_t, factory_t>::matches ? 0 : npos;
};

}  // namespace detail
}  // namespace arity

/*!
    searches factory for largest arity call that produces constructed, or searches constructed's ctors directly

    If `factory_t` is a callable type, `arity_v` investigates its call operators and resolves to the largest arity that
    returns a type converible to `constructed_t`. If `factory_t` is `void`, `arity_v` investigates `constructed_t`'s
    constructors directly instead.

    resolves to discovered arity or `npos`
*/
template <typename constructed_t, typename factory_t = void>
inline constexpr auto arity_v =
    arity::detail::arity_f<constructed_t, factory_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

}  // namespace dink
