/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <type_traits>

namespace dink {

/*!
    filters matches that would eventually call a copy or move special member function

    If the composer were to try to resolve a copy or move ctor, it would recurse indefinitely and crash. This ignores
    cases where there is 1 deduced arg and its type is the same as the resolved type, mod cv-ref.
*/
template <typename deduced_t, typename resolved_t, int_t num_args>
concept smf_filter = !(num_args == 1 && std::is_same_v<std::remove_cv_t<deduced_t>, resolved_t>);

/*!
    resolves individual args

    arg_t deduces the type of particular arg and returns an instance resolved by a composer.

    Deduction uses a pair of overloaded, implicit conversion templates to determine if it matched a shared ref or a
    transient value. It gets by on a technicality: the ref overload must be const, the val overload must not be.

    First, this overloads the conversion templates unambiguously. If both were const or neither were const, they
    wouldn't compile. Second, when called from a mutable arg_t to match a value type, the mutable value overload is a
    better match than the const ref overload. Otherwise, the ref overload would always be chosen, even for value types.

    Dispatch is aware of the fact that arg_t must be mutable.
*/
template <typename resolved_t, typename composer_t, int_t num_args>
class arg_t
{
public:
    template <smf_filter<resolved_t, num_args> deduced_t>
    constexpr operator deduced_t()
    {
        return composer_.template resolve<deduced_t>();
    }

    template <smf_filter<resolved_t, num_args> deduced_t>
    constexpr operator deduced_t&() const
    {
        return composer_.template resolve<deduced_t&>();
    }

    explicit constexpr arg_t(composer_t& composer) noexcept : composer_{composer} {}

private:
    composer_t& composer_;
};

} // namespace dink
