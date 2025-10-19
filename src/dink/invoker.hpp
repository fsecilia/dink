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

//! consumes indices to produce args
template <typename arg_t, typename single_arg_t>
struct indexed_arg_factory_t {
    template <std::size_t arity, std::size_t index>
    constexpr auto create(auto& container) const noexcept -> auto  // returns different types based on arity
    {
        if constexpr (arity == 1) return single_arg_t{arg_t{container}};
        else return arg_t{container};
    }
};

//! invokes a ctor or factory by replacing an index sequence with indexed args
template <typename constructed_t, typename indexed_arg_factory_t, typename index_sequence_t>
class indexed_invoker_t;

//! specialization to peel indices out of index sequence
template <typename constructed_t, typename indexed_arg_factory_t, std::size_t... indices>
class indexed_invoker_t<constructed_t, indexed_arg_factory_t, std::index_sequence<indices...>> {
public:
    constexpr auto create_value(auto& instance_factory, auto& container) const -> constructed_t {
        return instance_factory(indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...);
    }

    constexpr auto create_shared(auto& instance_factory, auto& container) const -> std::shared_ptr<constructed_t> {
        return std::make_shared<constructed_t>(
            instance_factory(indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...));
    }

    constexpr auto create_unique(auto& instance_factory, auto& container) const -> std::unique_ptr<constructed_t> {
        return std::make_unique<constructed_t>(
            instance_factory(indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...));
    }

    constexpr auto create_value(auto& container) const -> constructed_t {
        return constructed_t{indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...};
    }

    constexpr auto create_shared(auto& container) const -> std::shared_ptr<constructed_t> {
        return std::make_shared<constructed_t>(
            indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...);
    }

    constexpr auto create_unique(auto& container) const -> std::unique_ptr<constructed_t> {
        return std::make_unique<constructed_t>(
            indexed_arg_factory_.template create<sizeof...(indices), indices>(container)...);
    }

    explicit indexed_invoker_t(indexed_arg_factory_t indexed_arg_factory) noexcept
        : indexed_arg_factory_{std::move(indexed_arg_factory)} {}

    indexed_invoker_t() = default;

private:
    indexed_arg_factory_t indexed_arg_factory_{};
};

struct invoker_factory_t {
    template <typename constructed_t, typename resolved_factory_t, typename dependency_chain_t, lifetime_t min_lifetime,
              typename container_t>
    auto create() -> auto {
        using arg_t                 = arg_t<container_t, dependency_chain_t, min_lifetime>;
        using single_arg_t          = single_arg_t<constructed_t, arg_t>;
        using indexed_arg_factory_t = indexed_arg_factory_t<arg_t, single_arg_t>;

        static constexpr auto arity = arity_v<constructed_t, resolved_factory_t>;
        using indexed_invoker_t =
            indexed_invoker_t<constructed_t, indexed_arg_factory_t, std::make_index_sequence<arity>>;

        return indexed_invoker_t{};
    }
};

}  // namespace dink
