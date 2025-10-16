/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/not_found.hpp>
#include <utility>

namespace dink {

namespace factory_invoker::detail {

//! consumes indices to produce args backed by a container
template <typename arg_t, typename single_arg_t>
struct indexed_arg_factory_t {
    template <std::size_t arity, std::size_t index>
    static constexpr auto create(auto& container) -> auto  // returns different types based on arity
    {
        if constexpr (arity == 1) return single_arg_t{arg_t{container}};
        else return arg_t{container};
    }
};

//! converts an index sequence to args and invokes an instance factory with them
template <typename constructed_t, typename indexed_arg_factory_t, typename index_sequence_t>
struct arity_dispatcher_t;

template <typename constructed_t, typename indexed_arg_factory_t, std::size_t... indices>
struct arity_dispatcher_t<constructed_t, indexed_arg_factory_t, std::index_sequence<indices...>> {
    constexpr auto invoke_factory(auto& instance_factory, auto& container) const -> constructed_t {
        return instance_factory(indexed_arg_factory_t{}.template create<sizeof...(indices), indices>(container)...);
    }

    constexpr auto invoke_ctor(auto& container) const -> constructed_t {
        return constructed_t{indexed_arg_factory_t{}.template create<sizeof...(indices), indices>(container)...};
    }
};

}  // namespace factory_invoker::detail

//! invokes a constructed_t factory using arity to determine how many args backed by a container to pass, and what type.
template <typename constructed_t, std::size_t arity, typename arg_t, typename single_arg_t>
class invoker_t {
    static_assert(npos != arity, "could not deduce arity");

    using indexed_arg_factory_t = factory_invoker::detail::indexed_arg_factory_t<arg_t, single_arg_t>;
    using arity_dispatcher_t    = factory_invoker::detail::arity_dispatcher_t<constructed_t, indexed_arg_factory_t,
                                                                              std::make_index_sequence<arity>>;

public:
    constexpr auto invoke_factory(auto& factory, auto& container) const -> constructed_t {
        return arity_dispatcher_t{}.invoke_factory(factory, container);
    }

    constexpr auto invoke_ctor(auto& container) const -> constructed_t {
        return arity_dispatcher_t{}.invoke_ctor(container);
    }
};

}  // namespace dink
