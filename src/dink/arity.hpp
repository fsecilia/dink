/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arg.hpp>
#include <dink/meta.hpp>
#include <utility>

namespace dink {

namespace arity {

//! sentinel value used to indicate deduction failed
static inline constexpr auto arity_not_found = static_cast<std::size_t>(-1);

//! factory that forwards directly to ctors; this adapts direct ctor calls to the generic discoverable factory api
template <typename constructed_t>
class ctor_factory_t
{
public:
    template <typename... args_t>
    requires std::constructible_from<constructed_t, args_t...>
    auto operator()(args_t&&... args) -> constructed_t
    {
        return constructed_t{std::forward<args_t>(args)...};
    }
};

namespace detail {

struct probe_arg_t
{
    template <typename deduced_t>
    operator deduced_t();
};

template <typename constructed_t>
struct single_probe_arg_t
{
    template <single_arg_deducible<constructed_t> deduced_t>
    operator deduced_t();
};

template <typename constructed_t, typename factory_t, typename index_sequence_t>
struct arity_f;

// general case
template <typename constructed_t, typename factory_t, std::size_t index, std::size_t... remaining_indices>
struct arity_f<constructed_t, factory_t, std::index_sequence<index, remaining_indices...>>
{
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        constexpr auto current_arity = sizeof...(remaining_indices) + 1;
        using arg_t = std::conditional_t<current_arity == 1, single_probe_arg_t<constructed_t>, probe_arg_t>;
        using next_arity_f = arity_f<constructed_t, factory_t, std::index_sequence<remaining_indices...>>;
        if constexpr (std::is_invocable_v<factory_t, arg_t, meta::indexed_type_t<probe_arg_t, remaining_indices>...>)
        {
            if constexpr (std::is_convertible_v<
                              constructed_t,
                              std::invoke_result_t<
                                  factory_t, arg_t, meta::indexed_type_t<probe_arg_t, remaining_indices>...>>)
            {
                return current_arity;
            }
            else { return next_arity_f::value; }
        }
        else { return next_arity_f::value; }
    }();
};

// base case
template <typename constructed_t, typename factory_t>
struct arity_f<constructed_t, factory_t, std::index_sequence<>>
{
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        if constexpr (std::is_invocable_v<factory_t>)
        {
            if constexpr (std::is_convertible_v<constructed_t, std::invoke_result_t<factory_t>>) { return 0; }
            else { return arity_not_found; }
        }
        else { return arity_not_found; }
    }();
};

} // namespace detail
} // namespace arity

template <typename constructed_t, typename factory_t = arity::ctor_factory_t<constructed_t>>
inline constexpr auto arity_v
    = arity::detail::arity_f<constructed_t, factory_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

} // namespace dink
