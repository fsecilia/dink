/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "ctor.hpp"
#include <dink/test.hpp>
#include <dink/meta.hpp>
#include <type_traits>

namespace dink {
// namespace {

struct arg_t
{
    template <typename deduced_t>
    operator deduced_t();
};

template <typename deduced_t, typename constructed_t>
concept single_arg_bindable = !std::same_as<std::decay_t<deduced_t>, constructed_t>;

template <typename constructed_t>
struct single_arg_t
{
    template <single_arg_bindable<constructed_t> deduced_t>
    operator deduced_t();
};

// sentinel value used to indicate deduction failed
static inline constexpr auto ctor_not_found = static_cast<std::size_t>(-1);

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_f;

// general case
template <typename constructed_t, std::size_t index, std::size_t... remaining_indices>
struct ctor_arity_f<constructed_t, std::index_sequence<index, remaining_indices...>>
{
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        if constexpr (std::is_constructible_v<constructed_t, arg_t, meta::indexed_type_t<arg_t, remaining_indices>...>)
        {
            return sizeof...(remaining_indices) + 1;
        }
        else { return ctor_arity_f<constructed_t, std::index_sequence<remaining_indices...>>::value; }
    }();
};

// single arg case
template <typename constructed_t, std::size_t index>
struct ctor_arity_f<constructed_t, std::index_sequence<index>>
{
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        if constexpr (std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>) { return 1; }
        else { return ctor_arity_f<constructed_t, std::index_sequence<>>::value; }
    }();
};

// base case
template <typename constructed_t>
struct ctor_arity_f<constructed_t, std::index_sequence<>>
{
    static constexpr std::size_t const value = std::is_default_constructible_v<constructed_t> ? 0 : ctor_not_found;
};

template <typename constructed_t>
inline constexpr auto ctor_arity_v = []() noexcept -> std::size_t {
    constexpr auto result = ctor_arity_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;
    static_assert(result != ctor_not_found, "could not deduce ctor arity");
    return result;
}();

template <typename... args_t>
struct constructed_by_t
{
    constructed_by_t(args_t...) noexcept {}
};

static_assert(ctor_arity_v<constructed_by_t<>> == 0);
static_assert(ctor_arity_v<constructed_by_t<int>> == 1);
static_assert(ctor_arity_v<constructed_by_t<int*, float>> == 2);
static_assert(ctor_arity_v<constructed_by_t<void*, constructed_by_t<>, int>> == 3);
static_assert(ctor_arity_v<constructed_by_t<void*, constructed_by_t<>, int, float>> == 4);
static_assert(ctor_arity_v<constructed_by_t<void*, constructed_by_t<>, int, float, bool>> == 5);

struct multi_ctor_t
{
    multi_ctor_t();
    multi_ctor_t(int);
    multi_ctor_t(int, float, double);
};
static_assert(ctor_arity_v<multi_ctor_t> == 3);

// } // namespace
} // namespace dink
