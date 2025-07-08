/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

/*!
    maps from deduced or requested types to resolved types

    type_map_t is a customization point to replace a requested type during resolution. Clients use it to specify which
    concrete implementation should resolve an abstract type.
*/
template <typename requested_t>
struct type_map_t
{
    using result_t = requested_t;
};

//! literal type found in type_map_t
template <typename requested_t>
using mapped_type_t = typename type_map_t<requested_t>::result_t;

// ------------------------------------------------------------------------------------------------------------------ //

/*
    All requests go through the map, so type_map_t is also used to canonicalize refness and constness into either a
    simple mutable value or a mutable ref.
*/

template <typename requested_t>
struct type_map_t<requested_t&>
{
    using result_t = mapped_type_t<requested_t>&;
};

template <typename requested_t>
struct type_map_t<requested_t&&> : type_map_t<requested_t>
{};

template <typename requested_t>
struct type_map_t<requested_t const> : type_map_t<requested_t>
{};

template <typename requested_t>
struct type_map_t<requested_t const&> : type_map_t<requested_t&>
{};

template <typename requested_t>
struct type_map_t<requested_t const&&> : type_map_t<requested_t&&>
{};

} // namespace dink
