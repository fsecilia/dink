/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <optional>

namespace dink::bindings {

template <typename resolved_t>
class transient_t
{
public:
    auto bind(resolved_t resolved) -> void { resolved_ = std::move(resolved); }

    constexpr auto unbind() noexcept -> void { resolved_.reset(); }
    constexpr auto is_bound() const noexcept -> bool { return resolved_.has_value(); }

    constexpr auto bound() const noexcept -> resolved_t
    {
        assert(is_bound());
        return *resolved_;
    }

private:
    std::optional<resolved_t> resolved_{};
};

template <typename resolved_t>
class shared_t
{
public:
    auto bind(resolved_t const& resolved) -> void
    {
        assert(binding_t::unbound == binding_);
        binding_ = binding_t::const_;
        resolved_ = &resolved;
    }

    auto bind(resolved_t& resolved) -> void
    {
        assert(binding_t::unbound == binding_ || binding_t::mutable_ == binding_);
        binding_ = binding_t::mutable_;
        resolved_ = &resolved;
    }

    constexpr auto unbind() noexcept -> void { resolved_ = nullptr; }
    constexpr auto is_bound() const noexcept -> bool { return resolved_; }

    template <typename immediate_resolved_t>
    constexpr auto bound() const noexcept -> immediate_resolved_t&
    {
        assert(is_bound());

        assert(binding_t::unbound != binding_);
        if constexpr (!std::is_const_v<std::remove_reference_t<immediate_resolved_t>>)
        {
            assert(binding_t::mutable_ == binding_);
        }

        return const_cast<immediate_resolved_t&>(*resolved_);
    }

private:
    resolved_t const* resolved_{};

    enum class binding_t
    {
        unbound,
        const_,
        mutable_
    };
    binding_t binding_ = binding_t::unbound;
};

} // namespace dink::bindings
