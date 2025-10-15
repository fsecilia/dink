/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "canonical.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// arbitrary unique type
struct type_t {};

// deleter for unique_ptrs
struct deleter_t {};

// arbitrary size for arrays
constexpr auto array_size = 10;

// return type for functions
struct return_t {};

// arg type for functions
struct arg_1_t {};

// arg type for functions
struct arg_2_t {};

// basic types
static_assert(std::is_same_v<canonical_t<type_t>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t&>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t&&>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t const>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t volatile>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t*>, type_t>);

// function types
static_assert(std::is_same_v<canonical_t<void()>, void (*)()>);
static_assert(std::is_same_v<canonical_t<void (*)()>, void (*)()>);
static_assert(std::is_same_v<canonical_t<return_t()>, return_t (*)()>);
static_assert(std::is_same_v<canonical_t<return_t (*)()>, return_t (*)()>);
static_assert(std::is_same_v<canonical_t<void(arg_1_t)>, void (*)(arg_1_t)>);
static_assert(std::is_same_v<canonical_t<void (*)(arg_1_t)>, void (*)(arg_1_t)>);
static_assert(std::is_same_v<canonical_t<return_t(arg_1_t)>, return_t (*)(arg_1_t)>);
static_assert(std::is_same_v<canonical_t<return_t (*)(arg_1_t)>, return_t (*)(arg_1_t)>);
static_assert(std::is_same_v<canonical_t<void(arg_1_t, arg_2_t)>, void (*)(arg_1_t, arg_2_t)>);
static_assert(std::is_same_v<canonical_t<void (*)(arg_1_t, arg_2_t)>, void (*)(arg_1_t, arg_2_t)>);
static_assert(std::is_same_v<canonical_t<return_t(arg_1_t, arg_2_t)>, return_t (*)(arg_1_t, arg_2_t)>);
static_assert(std::is_same_v<canonical_t<return_t (*)(arg_1_t, arg_2_t)>, return_t (*)(arg_1_t, arg_2_t)>);

// array types
static_assert(std::is_same_v<canonical_t<type_t[]>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t const[]>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t[array_size]>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t const[array_size]>, type_t>);

// composite types
static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::unique_ptr<type_t>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::unique_ptr<type_t, deleter_t>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::shared_ptr<type_t>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::weak_ptr<type_t>>, type_t>);

// type combinations
static_assert(std::is_same_v<canonical_t<type_t const&>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t volatile*>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t const*&>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t**>, type_t>);
static_assert(std::is_same_v<canonical_t<type_t const* volatile*&&>, type_t>);
static_assert(std::is_same_v<canonical_t<return_t (*const)()>, return_t (*)()>);
static_assert(std::is_same_v<canonical_t<return_t (*volatile&)(arg_1_t)>, return_t (*)(arg_1_t)>);
static_assert(std::is_same_v<canonical_t<return_t (*&&)(arg_1_t, arg_2_t)>, return_t (*)(arg_1_t, arg_2_t)>);

static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t&>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t const&>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t> const>, type_t>);
static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t const>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::reference_wrapper<type_t const> const>, type_t>);

static_assert(std::is_same_v<canonical_t<std::unique_ptr<type_t> const&>, type_t>);
static_assert(std::is_same_v<canonical_t<std::shared_ptr<type_t*>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::shared_ptr<type_t const[]>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::shared_ptr<type_t const[array_size]> const&>, type_t>);

static_assert(std::is_same_v<canonical_t<std::weak_ptr<type_t const>>, type_t>);
static_assert(std::is_same_v<canonical_t<std::weak_ptr<type_t> const>, type_t>);
static_assert(std::is_same_v<canonical_t<std::weak_ptr<type_t const> const>, type_t>);

static_assert(std::is_same_v<canonical_t<std::unique_ptr<std::reference_wrapper<type_t const&>> const&>, type_t>);

}  // namespace
}  // namespace dink
