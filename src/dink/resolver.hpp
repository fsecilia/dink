// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/meta.hpp>
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

//! Resolves any argument type to produce an instance from a container.
//
// This is a match-any type that uses conversion operator templates to deduce
// the target type when passed as an argument to a function
//
// This type relies on specific interactions in overload resolution. It
// requires both a non-const value conversion operator and a const reference
// conversion operator. The const-qualified, reference-returning operator must
// be present to enable conversion to reference types at all. Its
// const-qualification is necessary to avoid ambiguity when both operators are
// instantiated with the same Deduced type for value conversions. For mutable
// lvalue references, both operators become viable (via template argument
// deduction of Deduced& for the value operator), but the non-const operator is
// eventually selected due to a better implicit object parameter conversion
// sequence.
//
// Both operators are required, the reference version must be const to avoid
// ambiguity, and the non-const version is selected for mutable references.
//
// This type is not fit to match single-argument ctors, because it will match
// copy and move ctors. That is handled by \ref SingleArgResolver.
template <typename Container>
class Resolver {
 public:
  //! Value conversion operator.
  //
  // This conversion operator matches everything but lvalue refs, including
  // rvalue refs and pointers.
  //
  // This method is NOT const to break ties in overload resolution, even though
  // it normally should be.
  template <typename Deduced>
  constexpr operator Deduced() {
    return resolve<Deduced, Canonical<Deduced>>();
  }

  //! Reference conversion operator.
  //
  // This conversion matches only lvalue refs.
  template <typename Deduced>
  constexpr operator Deduced&() const {
    return resolve<Deduced&, Canonical<Deduced>>();
  }

  explicit constexpr Resolver(Container& container) noexcept
      : container_{container} {}

 private:
  Container& container_;

  template <typename Deduced, typename CanonicalDeduced>
  constexpr auto resolve() const -> Deduced {
    return container_.template resolve<Deduced>();
  }
};

//! Wraps Resolver to exclude matching Constructed's copy or move ctors
template <typename Constructed, typename Resolver>
class SingleArgResolver {
 public:
  //! Value conversion operator.
  //
  // Deliberately not const for the same reason as in Resolver.
  //
  // /sa Resolver::operator Deduced().
  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  constexpr operator Deduced() {
    return resolver_.operator Deduced();
  }

  //! Reference conversion operator.
  //
  // /sa Resolver::operator Deduced&() const.
  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  constexpr operator Deduced&() const {
    return resolver_.operator Deduced&();
  }

  explicit constexpr SingleArgResolver(Resolver resolver) noexcept
      : resolver_{std::move(resolver)} {}

 private:
  Resolver resolver_;
};

//! Sequence that consumes indices to produce Resolvers.
template <template <typename Container> typename ResolverTemplate,
          template <typename Constructed,
                    typename Resolver> typename SingleArgResolverTemplate>
struct ResolverSequence {
  //! Creates a resolver sequence element, choosing the type based on arity.
  //
  // For arity 1, this creates a \c SingleArgResolver. For all other arities,
  // it creates a \c Resolver.
  template <typename Constructed, std::size_t arity, std::size_t index,
            typename Container>
  constexpr auto create_element(Container& container) const noexcept -> auto {
    using Resolver = ResolverTemplate<Container>;
    using SingleArgResolver = SingleArgResolverTemplate<Constructed, Resolver>;
    if constexpr (arity == 1) {
      return SingleArgResolver{Resolver{container}};
    } else {
      return Resolver{container};
    }
  }
};

template <typename ResolverSequence =
              ResolverSequence<Resolver, SingleArgResolver>>
struct ResolverSequenceFactory {
  constexpr auto create() const noexcept -> ResolverSequence { return {}; }
};

}  // namespace dink
