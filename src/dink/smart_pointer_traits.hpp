/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <type_traits>

namespace dink {

template <typename>
struct is_unique_ptr_f : std::false_type
{};

template <typename element_t, typename deleter_t>
struct is_unique_ptr_f<std::unique_ptr<element_t, deleter_t>> : std::true_type
{};

template <typename value_t>
constexpr bool is_unique_ptr_v = is_unique_ptr_f<std::remove_cvref_t<value_t>>::value;

template <typename value_t> concept is_unique_ptr = is_unique_ptr_v<value_t>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename>
struct is_shared_ptr_f : std::false_type
{};

template <typename element_t>
struct is_shared_ptr_f<std::shared_ptr<element_t>> : std::true_type
{};

template <typename value_t>
constexpr bool is_shared_ptr_v = is_shared_ptr_f<std::remove_cvref_t<value_t>>::value;

template <typename value_t> concept is_shared_ptr = is_shared_ptr_v<value_t>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename>
struct is_weak_ptr_f : std::false_type
{};

template <typename element_t>
struct is_weak_ptr_f<std::weak_ptr<element_t>> : std::true_type
{};

template <typename value_t>
constexpr bool is_weak_ptr_v = is_weak_ptr_f<value_t>::value;

template <typename value_t> concept is_weak_ptr = is_weak_ptr_v<value_t>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename source_t>
constexpr auto element_type(source_t&& source) -> decltype(auto)
{
    if constexpr (std::is_pointer_v<std::remove_cvref_t<source_t>> || is_shared_ptr_v<source_t>) return *source;
    else return std::forward<source_t>(source);
}

} // namespace dink
