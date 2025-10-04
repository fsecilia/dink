/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "binding.hpp"
#include <dink/test.hpp>

namespace dink::binding {
namespace {

constexpr int_t default_id = -1;
constexpr int_t expected_id = 3;

struct interface_t
{
    int_t id = default_id;
};

struct implementation_t : interface_t
{};

struct factory_t
{
    int_t id = default_id;
};

// ---------------------------------------------------------------------------------------------------------------------

// all scope combinations

static_assert(constraints::valid_scope_v<providers::type_t<interface_t>, scopes::transient_t>);
static_assert(constraints::valid_scope_v<providers::type_t<interface_t>, scopes::scoped_t>);
static_assert(constraints::valid_scope_v<providers::type_t<interface_t>, scopes::singleton_t>);
static_assert(constraints::valid_scope_v<providers::factory_t<factory_t>, scopes::transient_t>);
static_assert(constraints::valid_scope_v<providers::factory_t<factory_t>, scopes::scoped_t>);
static_assert(constraints::valid_scope_v<providers::factory_t<factory_t>, scopes::singleton_t>);
static_assert(!constraints::valid_scope_v<providers::external_instance_t<interface_t>, scopes::transient_t>);
static_assert(!constraints::valid_scope_v<providers::external_instance_t<interface_t>, scopes::scoped_t>);
static_assert(constraints::valid_scope_v<providers::external_instance_t<interface_t>, scopes::singleton_t>);
static_assert(!constraints::valid_scope_v<providers::internal_instance_t<interface_t>, scopes::transient_t>);
static_assert(!constraints::valid_scope_v<providers::internal_instance_t<interface_t>, scopes::scoped_t>);
static_assert(constraints::valid_scope_v<providers::internal_instance_t<interface_t>, scopes::singleton_t>);
static_assert(constraints::valid_scope_v<providers::external_prototype_t<interface_t>, scopes::transient_t>);
static_assert(!constraints::valid_scope_v<providers::external_prototype_t<interface_t>, scopes::scoped_t>);
static_assert(!constraints::valid_scope_v<providers::external_prototype_t<interface_t>, scopes::singleton_t>);
static_assert(constraints::valid_scope_v<providers::internal_prototype_t<interface_t>, scopes::transient_t>);
static_assert(!constraints::valid_scope_v<providers::internal_prototype_t<interface_t>, scopes::scoped_t>);
static_assert(!constraints::valid_scope_v<providers::internal_prototype_t<interface_t>, scopes::singleton_t>);

// ---------------------------------------------------------------------------------------------------------------------

// default scopes

static_assert(
    std::is_same_v<typename decltype(bind<interface_t>().to<implementation_t>())::scope_t, scopes::transient_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_factory(std::declval<factory_t>()))::scope_t, scopes::transient_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_external_instance(std::declval<implementation_t&>()))::scope_t,
        scopes::singleton_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_external_instance(std::declval<implementation_t const&>()))::scope_t,
        scopes::singleton_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_internal_instance(std::declval<implementation_t>()))::scope_t,
        scopes::singleton_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_external_prototype(std::declval<implementation_t const&>()))::scope_t,
        scopes::transient_t>
);
static_assert(
    std::is_same_v<
        typename decltype(bind<interface_t>().to_internal_prototype(std::declval<implementation_t>()))::scope_t,
        scopes::transient_t>
);

// .in_scope() replaces scope, but keeps interface and provider

static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::scoped_t>())::interface_t, interface_t>
);
static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::scoped_t>())::provider_t,
        providers::type_t<implementation_t>>
);
static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::scoped_t>())::scope_t, scopes::scoped_t>
);

static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::singleton_t>())::interface_t, interface_t>
);
static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::singleton_t>())::provider_t,
        providers::type_t<implementation_t>>
);
static_assert(
    std::is_same_v<
        decltype(bind<interface_t>().to<implementation_t>().in_scope<scopes::singleton_t>())::scope_t,
        scopes::singleton_t>
);

// values passed are values used

constexpr auto instance = implementation_t{expected_id};
static_assert(bind<interface_t>().to_factory<factory_t>(factory_t{expected_id}).provider.factory.id == expected_id);
static_assert(bind<interface_t>().to_external_instance(instance).provider.instance == &instance);
static_assert(
    bind<interface_t>().to_external_instance(static_cast<implementation_t const&>(instance)).provider.instance->id
    == expected_id
);
static_assert(bind<interface_t>().to_internal_instance(instance).provider.instance.id == expected_id);
static_assert(bind<interface_t>().to_external_prototype(instance).provider.prototype == &instance);
static_assert(bind<interface_t>().to_internal_prototype(instance).provider.prototype.id == expected_id);

} // namespace
} // namespace dink::binding
