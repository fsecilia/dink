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
    auto find_in_parent(on_found_t&&, on_not_found_t&& on_not_found) -> decltype(auto) {
        return on_not_found();
    }
};

// nested container delegates to parent, passing continuations through
template <typename parent_container_t>
struct to_parent_t {
    parent_container_t* parent_container;

    template <typename request_t, typename on_found_t, typename on_not_found_t>
    auto find_in_parent(on_found_t&& on_found, on_not_found_t&& on_not_found) -> decltype(auto) {
        return parent_container->template resolve_or_delegate<request_t>(std::forward<on_found_t>(on_found),
                                                                         std::forward<on_not_found_t>(on_not_found));
    }

    explicit to_parent_t(parent_container_t& parent) : parent_container{&parent} {}
};

}  // namespace dink::delegate
