/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <memory>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

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
    //! returns a previously resolved instance, searching locally then in parent, or nullptr if not found
    template <typename resolved_t>
    auto resolved() const noexcept -> resolved_t*
    {
        auto const result = instance_map_.find(key<resolved_t>());
        if (result != instance_map_.end())
        {
            auto& instance = *result->second;
            return &instance.template downcast_resolved<resolved_t>();
        }

        return parent_->template resolved<resolved_t>();
    }

    //! returns existing, searching locally then in parent, or resolves through composer and saves for future use
    template <typename resolved_t, typename composer_t>
    auto resolve(composer_t& composer) -> resolved_t&
    {
        auto const result = resolved<resolved_t>();
        if (result) return *result;
        return resolve_locally<resolved_t>(composer);
    }

    constexpr auto parent() const noexcept -> parent_t const& { return *parent_; }
    constexpr auto parent() noexcept -> parent_t& { return *parent_; }

    explicit constexpr local_t(parent_t& parent) noexcept : parent_{&parent} {}
    ~local_t() { destroy_in_reverse_order(); }

    local_t(local_t&& src) noexcept = default;
    auto operator=(local_t&& src) noexcept -> local_t& = default;

private:
    struct instance_t
    {
        template <typename resolved_t>
        auto downcast_resolved() noexcept -> resolved_t&
        {
            return static_cast<resolved_instance_t<resolved_t>&>(*this).resolved;
        }

        virtual ~instance_t() = default;
    };

    template <typename resolved_t>
    struct resolved_instance_t : instance_t
    {
        resolved_t resolved;
        resolved_instance_t(resolved_t resolved) noexcept : resolved{std::move(resolved)} {}
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
        // append new instance to vector
        auto resolved_instance
            = std::make_unique<resolved_instance_t<resolved_t>>(composer.template resolve<resolved_t>());
        auto& instance = *instances_.emplace_back(std::move(resolved_instance)).get();

        // insert into map or roll back vector
        try
        {
            instance_map_.emplace(key<resolved_t>(), &instance);
        }
        catch (...)
        {
            instances_.pop_back();
            throw;
        }

        return instance.template downcast_resolved<resolved_t>();
    }

    /*!
        destroys elements from back to front

        Astonishingly, circa 2025q3, I cannot find a guarantee that vector specifically, or sequence containers in
        general, must destroy elements in reverse order from their destructors. It seems this would be guaranteed by
        the standard, but there is nothing specific to be found there. Internet searches, forum posts, and AI are all
        inconclusive. This is easy enough to enforce here manually, though, so we just do it.
    */
    auto destroy_in_reverse_order() noexcept -> void
    {
        for (auto& instance : instances_ | std::views::reverse) instance.reset();
    }

    std::unordered_map<key_t, instance_t*> instance_map_{};
    std::vector<std::unique_ptr<instance_t>> instances_{};
    parent_t* parent_;
};

} // namespace dink::scope
