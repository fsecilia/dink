/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "ctor.hpp"
#include <dink/test.hpp>
#include <dink/meta.hpp>
#include <dink/tuple.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

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

#if 0

// this version uses parspec and type packs, but requires an entry point to kick it all off

template <typename constructed_t, bool is_constructible, typename... args_t>
struct ctor_arity_f;

template <typename constructed_t, typename... args_t>
struct ctor_arity_f<constructed_t, true, args_t...>
{
    static constexpr std::size_t const value = sizeof...(args_t);
};

template <typename constructed_t, typename next_arg_t, typename... remaining_args_t>
struct ctor_arity_f<constructed_t, false, next_arg_t, remaining_args_t...>
{
    static constexpr std::size_t const value = ctor_arity_f<
        constructed_t, std::is_constructible_v<constructed_t, remaining_args_t...>, remaining_args_t...>::value;
};

template <typename constructed_t, typename next_arg_t, typename remaining_arg_t>
struct ctor_arity_f<constructed_t, false, next_arg_t, remaining_arg_t>
{
    static constexpr std::size_t const value = ctor_arity_f<
        constructed_t, std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>,
        single_arg_t<constructed_t>>::value;
};

template <typename constructed_t>
struct ctor_arity_f<constructed_t, false>
{
    static_assert(std::is_default_constructible_v<constructed_t>, "could not deduce constructor arity");
    static constexpr std::size_t const value = 0;
};

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_dispatcher_f;

template <typename constructed_t, std::size_t... indices>
struct ctor_arity_dispatcher_f<constructed_t, std::index_sequence<indices...>>
    : ctor_arity_f<
          constructed_t, std::is_constructible_v<constructed_t, meta::indexed_type_t<arg_t, indices>...>,
          meta::indexed_type_t<arg_t, indices>...>
{};

template <typename constructed_t>
inline constexpr auto const ctor_arity_v
    = ctor_arity_dispatcher_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

#endif

#if 0

// this is the shortest version, using a triply nested lambda to keep nearby logic nearby and index sequences everywhere

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_f;

template <typename constructed_t, std::size_t index, std::size_t... remaining_indices>
struct ctor_arity_f<constructed_t, std::index_sequence<index, remaining_indices...>>
{
private:
    using current_sequence_t = std::index_sequence<index, remaining_indices...>;

    static constexpr bool is_constructible_now_v = []() {
        if constexpr (sizeof...(remaining_indices) + 1 == 1)
        {
            return std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>;
        }
        else
        {
            return []<std::size_t... indices>(std::index_sequence<indices...>) {
                return std::is_constructible_v<constructed_t, meta::indexed_type_t<arg_t, indices>...>;
            }(current_sequence_t{});
        }
    }();

public:
    static constexpr std::size_t const value = [] {
        if constexpr (is_constructible_now_v) { return sizeof...(remaining_indices) + 1; }
        else { return ctor_arity_f<constructed_t, std::index_sequence<remaining_indices...>>::value; }
    }();
};

template <typename constructed_t>
struct ctor_arity_f<constructed_t, std::index_sequence<>>
{
    static constexpr std::size_t const value = std::is_constructible_v<constructed_t> ? 0 : 0;
};

template <typename constructed_t>
inline constexpr auto const ctor_arity_v
    = ctor_arity_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

#endif

#if 0

// this is similar to the lambda version, but uses named functions. it also removes an if constexpr by using an overload

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_f;

template <typename constructed_t, std::size_t index, std::size_t... remaining_indices>
struct ctor_arity_f<constructed_t, std::index_sequence<index, remaining_indices...>>
{
private:
    template <std::size_t... indices>
    static constexpr auto is_constructible(std::index_sequence<indices...>) noexcept -> bool
    {
        return std::is_constructible_v<constructed_t, meta::indexed_type_t<arg_t, indices>...>;
    }

    static constexpr auto is_constructible(std::index_sequence<index>) noexcept -> bool
    {
        return std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>;
    }

    static constexpr auto value_or_next() noexcept -> std::size_t
    {
        if constexpr (is_constructible(std::index_sequence<index, remaining_indices...>{}))
        {
            return sizeof...(remaining_indices) + 1;
        }
        else { return ctor_arity_f<constructed_t, std::index_sequence<remaining_indices...>>::value; }
    }

public:
    static constexpr std::size_t const value = value_or_next();
};

template <typename constructed_t>
struct ctor_arity_f<constructed_t, std::index_sequence<>>
{
    static_assert(std::is_default_constructible_v<constructed_t>, "could not deduce constructor arity");
    static constexpr std::size_t const value = 0;
};

template <typename constructed_t>
inline constexpr auto const ctor_arity_v
    = ctor_arity_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

#endif

#if 0

// this verison uses index sequences all the way down, but still uses partial specialization to remove the overload

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_f;

template <typename constructed_t, std::size_t index, std::size_t... remaining_indices>
struct ctor_arity_f<constructed_t, std::index_sequence<index, remaining_indices...>>
{
private:
    static constexpr auto value_or_next() noexcept -> std::size_t
    {
        if constexpr (std::is_constructible_v<constructed_t, arg_t, meta::indexed_type_t<arg_t, remaining_indices>...>)
        {
            return sizeof...(remaining_indices) + 1;
        }
        else { return ctor_arity_f<constructed_t, std::index_sequence<remaining_indices...>>::value; }
    }

public:
    static constexpr std::size_t const value = value_or_next();
};

template <typename constructed_t, std::size_t index>
struct ctor_arity_f<constructed_t, std::index_sequence<index>>
{
private:
    static constexpr auto value_or_next() noexcept -> std::size_t
    {
        if constexpr (std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>) { return 1; }
        else { return ctor_arity_f<constructed_t, std::index_sequence<>>::value; }
    }

public:
    static constexpr std::size_t const value = value_or_next();
};

template <typename constructed_t>
struct ctor_arity_f<constructed_t, std::index_sequence<>>
{
    static_assert(std::is_default_constructible_v<constructed_t>, "could not deduce constructor arity");
    static constexpr std::size_t const value = 0;
};

template <typename constructed_t>
inline constexpr auto const ctor_arity_v
    = ctor_arity_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

#endif

#if 1

// this verison uses index sequences all the way down and parspec, but one lambda per to get the if constexpr

template <typename constructed_t, typename index_sequence_t>
struct ctor_arity_f;

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

template <typename constructed_t, std::size_t index>
struct ctor_arity_f<constructed_t, std::index_sequence<index>>
{
    static constexpr std::size_t const value = []() noexcept -> std::size_t {
        if constexpr (std::is_constructible_v<constructed_t, single_arg_t<constructed_t>>) { return 1; }
        else { return ctor_arity_f<constructed_t, std::index_sequence<>>::value; }
    }();
};

template <typename constructed_t>
struct ctor_arity_f<constructed_t, std::index_sequence<>>
{
    static_assert(std::is_default_constructible_v<constructed_t>, "could not deduce constructor arity");
    static constexpr std::size_t const value = 0;
};

template <typename constructed_t>
inline constexpr auto const ctor_arity_v
    = ctor_arity_f<constructed_t, std::make_index_sequence<dink_max_deduced_arity>>::value;

#endif

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
