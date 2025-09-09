/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <utility>

namespace dink {

//! matches any argument type to produce an instance from a container; not fit to match single-argument ctors
template <typename container_t>
class arg_t
{
public:
    // this method is NOT const to break ties in overload resolution, even though it normally would be
    template <typename deduced_t>
    operator deduced_t()
    {
        return container_.template resolve<deduced_t>();
    }

    template <typename deduced_t>
    operator deduced_t&() const
    {
        return container_.template resolve<deduced_t&>();
    }

    explicit arg_t(container_t& container) noexcept : container_{container} {}

private:
    container_t& container_;
};

//! filters out signatures that match copy ctor or move ctor
template <typename deduced_t, typename resolved_t>
concept single_arg_deducible = !std::same_as<std::remove_cvref_t<deduced_t>, resolved_t>;

//! matches any argument type to produce an instance from a container; excludes signatures that match copy or move ctor
template <typename resolved_t, typename arg_t>
class single_arg_t
{
public:
    template <single_arg_deducible<resolved_t> deduced_t>
    operator deduced_t()
    {
        return static_cast<deduced_t>(arg_);
    }

    template <single_arg_deducible<resolved_t> deduced_t>
    operator deduced_t&() const
    {
        return static_cast<deduced_t&>(arg_);
    }

    explicit single_arg_t(arg_t arg) noexcept : arg_{std::move(arg)} {}

private:
    arg_t arg_;
};

} // namespace dink
