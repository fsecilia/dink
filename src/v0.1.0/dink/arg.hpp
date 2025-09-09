/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

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

} // namespace dink
