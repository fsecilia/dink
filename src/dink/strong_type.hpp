/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace dink {

/*!
    wraps t_value_t in a type unique to tag_t

    Dink resolves by type. When resolving multiple instances of the same type, either they will all resolve with the
    same value, or the concrete types need to be made distinct. strong_type_t imbues arbitrary types with sufficient
    uniqueness that it can distingusigh betwen them:

        static_assert(  typeid(strong_type_t<int, struct tag_1>) == typeid(strong_type_t<int, struct tag_1>) );
        static_assert(!(typeid(strong_type_t<int, struct tag_1>) == typeid(strong_type_t<int, struct tag_2>)));

    Note that tags need not actually be defined if you declare them as a struct, as done here.
*/
template <typename t_value_t, typename tag_t, typename... additional_tags_t>
class strong_type_t
{
public:
    using value_t = t_value_t;

    explicit constexpr operator value_t const&() const noexcept { return get(); }
    explicit constexpr operator value_t&() noexcept { return get(); }
    constexpr auto get() const noexcept -> value_t const& { return value_; }
    constexpr auto get() noexcept -> value_t& { return value_; }

    constexpr auto operator*() const noexcept -> value_t const& { return get(); }
    constexpr auto operator*() noexcept -> value_t& { return get(); }
    constexpr value_t const* operator->() const noexcept { return &get(); }
    constexpr value_t* operator->() noexcept { return get(); }

    constexpr auto operator<=>(strong_type_t const& src) const noexcept -> auto = default;

    explicit constexpr strong_type_t() noexcept(std::is_nothrow_constructible_v<value_t>) = default;
    constexpr strong_type_t(strong_type_t const& src) noexcept(std::is_nothrow_copy_constructible_v<value_t>) = default;
    constexpr strong_type_t(strong_type_t&& src) noexcept(std::is_nothrow_move_constructible_v<value_t>) = default;

    // variadic forwarding ctor
    template <typename... args_t>
    requires(std::is_constructible_v<value_t, args_t...>)
    explicit constexpr strong_type_t(args_t&&... args) noexcept(std::is_nothrow_constructible_v<value_t, args_t...>)
        : value_{std::forward<args_t>(args)...}
    {}

    // copy conversion
    template <typename other_value_t, typename... other_tags_t>
    requires(std::constructible_from<value_t, other_value_t>)
    explicit constexpr strong_type_t(strong_type_t<other_value_t, other_tags_t...> const& src) //
        noexcept(std::is_nothrow_constructible_v<value_t, other_value_t>)
        : value_{static_cast<value_t>(src.get())}
    {}

    // move conversion
    template <typename other_value_t, typename... other_tags_t>
    requires(std::constructible_from<value_t, other_value_t>)
    explicit constexpr strong_type_t(strong_type_t<other_value_t, other_tags_t...>&& src) //
        noexcept(std::is_nothrow_constructible_v<value_t, other_value_t>)
        : value_{static_cast<value_t>(std::move(src).get())}
    {}

    constexpr auto operator=(strong_type_t const& src) //
        noexcept(std::is_nothrow_copy_assignable_v<value_t>) -> strong_type_t&
    {
        if (this != &src) value_ = src.value_;
        return *this;
    }

    constexpr auto operator=(strong_type_t&& src) //
        noexcept(std::is_nothrow_move_assignable_v<value_t>) -> strong_type_t&
    {
        if (this != &src) value_ = std::move(src.value_);
        return *this;
    }

    template <typename... args_t>
    requires requires(args_t&&... args) {
        { value_t::construct(args...) } -> std::convertible_to<value_t>;
    }
    static auto construct(args_t&&... args) -> strong_type_t
    {
        return strong_type_t{value_t::construct(std::forward<args_t>(args)...)};
    }

private:
    value_t value_;
};

} // namespace dink
