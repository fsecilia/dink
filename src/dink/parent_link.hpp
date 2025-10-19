/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <utility>

namespace dink::parent_link {

// root container has no parent - executes the "not found" continuation
struct none_t {
    template <typename request_t, typename resolver_t, typename on_found_t, typename on_not_found_t>
    auto find_in_parent(resolver_t&, on_found_t&&, on_not_found_t&& on_not_found) -> decltype(auto) {
        return on_not_found();
    }
};

// nested container delegates to parent, passing continuations through
template <typename parent_container_t>
struct to_parent_t {
    parent_container_t* parent_container;

    template <typename request_t, typename resolver_t, typename on_found_t, typename on_not_found_t>
    auto find_in_parent(resolver_t& resolver, on_found_t&& on_found, on_not_found_t&& on_not_found) -> decltype(auto) {
        return parent_container->template resolve_hierarchically<request_t>(
            resolver, std::forward<on_found_t>(on_found), std::forward<on_not_found_t>(on_not_found));
    }

    explicit to_parent_t(parent_container_t& parent) : parent_container{&parent} {}
};

}  // namespace dink::parent_link
