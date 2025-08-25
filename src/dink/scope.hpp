/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace dink::scope {

//! root scope, resolves instances using a meyers singleton
class global_t
{
public:
    //! returns a previously resolved instance, or nullptr if not found
    template <typename resolved_t>
    constexpr auto resolved() const noexcept -> resolved_t*
    {
        return resolved_instance_t<resolved_t>::instance;
    }

    //! returns a previously resolved instance, or resolves one through the composer and saves it for future use
    template <typename resolved_t, typename composer_t>
    constexpr auto resolve(composer_t& composer) -> resolved_t&
    {
        if (!resolved_instance_t<resolved_t>::instance)
        {
            static auto instance = static_cast<resolved_t>(composer.template resolve<resolved_t>());
            resolved_instance_t<resolved_t>::instance = &instance;
        }

        return *resolved_instance_t<resolved_t>::instance;
    }

private:
    template <typename resolved_t>
    struct resolved_instance_t
    {
        inline static resolved_t* instance = nullptr;
    };
};

//! nested scope, chains to a parent scope, then resolves local instances using via hash table
template <typename parent_t>
class local_t
{
public:
    template <typename resolved_t>
    auto resolved() const noexcept -> resolved_t*
    {
        auto result = instances_.find(key<resolved_t>());
        if (result != instances_.end()) &result->second;
        return parent_->resolved();
    }

    template <typename resolved_t, typename composer_t>
    auto resolve(composer_t& composer) -> resolved_t&
    {
        auto resolved_by_parent_scope = resolved<resolved_t>();
        if (resolved_by_parent_scope) return *resolved_by_parent_scope;
        return resolve_locally<resolved_t>(composer);
    }

    constexpr auto parent() const noexcept -> parent_t const& { return *parent_; }
    constexpr auto parent() noexcept -> parent_t& { return *parent_; }

    explicit constexpr local_t(parent_t& parent) noexcept : parent_{&parent} {}

private:
    struct instance_t
    {
        virtual ~instance_t() = default;
    };

    template <typename resolved_t>
    struct resolved_instance_t : instance_t
    {
        resolved_t resolved;
        ~resolved_instance_t() override = default;
    };

    using key_t = std::type_index;

    template <typename resolved_t>
    constexpr auto key() const noexcept -> key_t
    {
        return key_t{typeid(resolved_t)};
    }

    template <typename resolved_t, typename composer_t>
    auto resolve_locally(composer_t& composer) -> resolved_t&
    {
        return std::get<resolved_t>(instances_.emplace(
            key<resolved_t>(),
            resolved_instance_t{.resolved = static_cast<resolved_t>(composer.template resolve<resolved_t>())}
        ));
    }

    std::unordered_map<key_t, std::unique_ptr<instance_t>> instances_{};
    parent_t* parent_;
};

} // namespace dink::scope
