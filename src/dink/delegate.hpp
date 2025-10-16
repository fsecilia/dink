/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <type_traits>
#include <utility>

namespace dink::delegate {

// root container has no parent - executes the "not found" continuation
struct none_t {
    template <typename request_t, typename on_found_t, typename on_not_found_t>
    auto search(on_found_t&&, on_not_found_t&& on_not_found) -> decltype(auto) {
        return on_not_found();
    }
};

// nested container delegates to parent, passing continuations through
template <typename parent_container_t>
struct to_parent_t {
    parent_container_t* parent_container;

    template <typename request_t, typename on_found_t, typename on_not_found_t>
    auto search(on_found_t&& on_found, on_not_found_t&& on_not_found) -> decltype(auto) {
        return parent_container->template search<request_t>(std::forward<on_found_t>(on_found),
                                                            std::forward<on_not_found_t>(on_not_found));
    }

    explicit to_parent_t(parent_container_t& parent) : parent_container{&parent} {}
};

template <typename value_t>
struct is_delegate_f : std::false_type {};

template <>
struct is_delegate_f<delegate::none_t> : std::true_type {};

template <typename parent_t>
struct is_delegate_f<delegate::to_parent_t<parent_t>> : std::true_type {};

template <typename value_t>
concept is_delegate = is_delegate_f<std::remove_cvref_t<value_t>>::value;

}  // namespace dink::delegate
