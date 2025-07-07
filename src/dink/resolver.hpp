/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <type_traits>

namespace dink::resolvers {

//! transient requests are dispatched to a factory and returned directly
template <
    template <typename, typename, typename, template <typename, typename, int_t> class, typename...> class dispatcher_t,
    template <typename> class factory_t, template <typename, typename, int_t> class arg_t>
class transient_t
{
public:
    template <typename resolved_t, typename composer_t>
    auto resolve(composer_t& composer) const -> resolved_t
    {
        return dispatcher_t<resolved_t, composer_t, factory_t<resolved_t>, arg_t>{}(composer);
    }
};

//! shared requests remove refness, resolve via the composer, store the result in a static, then return the static
class shared_t
{
public:
    template <typename resolved_t, typename composer_t>
    auto resolve(composer_t& composer) const -> resolved_t&
    {
        using canonical_t = std::remove_cvref_t<resolved_t>;
        return shared_instance<canonical_t>(composer);
    }

private:
    template <typename canonical_t, typename composer_t>
    auto shared_instance(composer_t& composer) const -> canonical_t&
    {
        static auto shared_instance = static_cast<canonical_t>(composer.template resolve<canonical_t>());
        return shared_instance;
    }
};

} // namespace dink::resolvers
