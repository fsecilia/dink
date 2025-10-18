/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

// ---------------------------------------------------------------------------------------------------------------------
// metafunctions
// ---------------------------------------------------------------------------------------------------------------------

//! resolves type to a valid function return type by removing rvalue references
template <typename type_t>
struct as_returnable_f;

//! general case is passthrough
template <typename type_t>
struct as_returnable_f {
    using type = type_t;
};

//! rvalue reference specialization resolves to unqualified value type
template <typename type_t>
struct as_returnable_f<type_t&&> {
    using type = type_t;
};

//! alias resolves type to valid function return type
template <typename type_t>
using as_returnable_t = typename as_returnable_f<type_t>::type;

// ---------------------------------------------------------------------------------------------------------------------
// forward declaration
// ---------------------------------------------------------------------------------------------------------------------

template <typename requested_t>
struct request_traits_t;

//! type actually cached and provided for a given request
template <typename requested_t>
using resolved_t = typename request_traits_t<requested_t>::value_type;

// =====================================================================================================================
// Request Traits - instance-based, parameterized on request_t
// =====================================================================================================================

/*!
    request traits for value types (T)

    - Resolves to the type itself
    - Uses default scope behavior (respects bound scope)
    - Returns by value (copy or move)
*/
template <typename requested_t>
struct request_traits_t {
    using request_t  = requested_t;
    using value_type = requested_t;

    template <typename source_t>
    auto from_provided(source_t&& source) const -> requested_t {
        return element_type(std::forward<source_t>(source));
    }

    template <typename cached_t>
    auto from_lookup(cached_t&& cached) const -> requested_t {
        if constexpr (std::is_pointer_v<std::remove_cvref_t<cached_t>>) return std::move(*cached);
        else return *cached;
    }
};

/*!
    request traits for rvalue references (T&&)

    rvalue references are treated the same as value types
*/
template <typename requested_t>
struct request_traits_t<requested_t&&> : request_traits_t<requested_t> {};

/*!
    request traits for lvalue references (T&)

    - Always singleton (requires stable address)
    - Returns by lvalue reference
*/
template <typename requested_t>
struct request_traits_t<requested_t&> {
    using request_t  = requested_t&;
    using value_type = requested_t;

    template <typename source_t>
    auto from_provided(source_t&& source) const -> decltype(auto) {
        return element_type(std::forward<source_t>(source));
    }

    template <typename cached_t>
    auto from_lookup(cached_t&& cached) const -> requested_t& {
        return *cached;
    }
};

/*!
    request traits for raw pointers (T*)

    - Always singleton (pointer implies shared lifetime)
    - Returns address of cached instance
*/
template <typename requested_t>
struct request_traits_t<requested_t*> {
    using request_t  = requested_t*;
    using value_type = requested_t;

    template <typename source_t>
    auto from_provided(source_t&& source) const -> requested_t* {
        return &element_type(std::forward<source_t>(source));
    }

    template <typename cached_t>
    auto from_lookup(cached_t&& cached) const -> requested_t* {
        // cached is already T* from cache.get_instance<T>()
        return cached;
    }
};

/*!
    request traits for unique_ptr<T>

    - Always transient (unique ownership semantics)
    - Never cached (each request gets a new unique_ptr)
    - Wraps result in unique_ptr
*/
template <typename requested_t, typename deleter_t>
struct request_traits_t<std::unique_ptr<requested_t, deleter_t>> {
    using request_t  = std::unique_ptr<requested_t, deleter_t>;
    using value_type = std::remove_cvref_t<requested_t>;

    template <typename source_t>
    auto from_provided(source_t&& source) const -> std::unique_ptr<requested_t, deleter_t> {
        if constexpr (is_unique_ptr_v<std::remove_cvref_t<source_t>>) {
            return std::forward<source_t>(source);
        } else {
            return std::make_unique<requested_t>(element_type(std::forward<source_t>(source)));
        }
    }

    template <typename cached_t>
    auto from_lookup(cached_t&& cached) const -> std::unique_ptr<requested_t, deleter_t> {
        return std::make_unique<requested_t>(std::move(*cached));
    }
};

/*!
    request traits for shared_ptr<T>

    - Uses default scope behavior
    - Caches canonical shared_ptr instances
    - Returns shared ownership
*/
template <typename requested_t>
struct request_traits_t<std::shared_ptr<requested_t>> {
    using request_t  = std::shared_ptr<requested_t>;
    using value_type = std::remove_cvref_t<requested_t>;

    template <typename source_t>
    auto from_provided(source_t&& source) const -> std::shared_ptr<requested_t> {
        if constexpr (is_shared_ptr_v<std::remove_cvref_t<source_t>>) {
            return std::forward<source_t>(source);
        } else {
            return std::make_shared<requested_t>(std::forward<source_t>(source));
        }
    }

    template <typename source_t>
    auto from_lookup(source_t&& source) const -> std::shared_ptr<requested_t> {
        if constexpr (is_shared_ptr_v<std::remove_cvref_t<source_t>>) {
            return std::forward<source_t>(source);
        } else {
            return std::make_shared<requested_t>(element_type(std::forward<source_t>(source)));
        }
    }
};

/*!
    request traits for weak_ptr<T>

    - Always singleton (weak_ptr requires cached shared_ptr)
    - Leverages shared_ptr caching
    - Returns weak reference to cached shared_ptr
*/
template <typename requested_t>
struct request_traits_t<std::weak_ptr<requested_t>> : request_traits_t<std::shared_ptr<requested_t>> {
    using request_t = std::weak_ptr<requested_t>;  // override the inherited typedef
};

//! request traits for const value types - delegates to non-const version
template <typename requested_t>
struct request_traits_t<requested_t const> : request_traits_t<requested_t> {
    using request_t = requested_t const;
};

//! request traits for const lvalue references - delegates to non-const reference version
template <typename requested_t>
struct request_traits_t<requested_t const&> : request_traits_t<requested_t&> {
    using request_t = requested_t const&;
};

//! request traits for const pointers - delegates to non-const pointer version
template <typename requested_t>
struct request_traits_t<requested_t const*> : request_traits_t<requested_t*> {
    using request_t = requested_t const*;
};

template <typename requested_t, typename deleter_t>
struct request_traits_t<std::unique_ptr<requested_t const, deleter_t>>
    : request_traits_t<std::unique_ptr<requested_t, deleter_t>> {
    using request_t = std::unique_ptr<requested_t const, deleter_t>;
};

template <typename requested_t>
struct request_traits_t<std::shared_ptr<requested_t const>> : request_traits_t<std::shared_ptr<requested_t>> {
    using request_t = std::shared_ptr<requested_t const>;
};

template <typename requested_t>
struct request_traits_t<std::weak_ptr<requested_t const>> : request_traits_t<std::weak_ptr<requested_t>> {
    using request_t = std::weak_ptr<requested_t const>;
};

}  // namespace dink
