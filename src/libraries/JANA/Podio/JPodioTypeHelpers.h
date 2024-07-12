
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once
#include <type_traits>

#include <podio/podioVersion.h>
#include <JANA/JVersion.h>

/// These allow us to have both a PODIO-enabled and a PODIO-free definition of certain key structures and functions.

#if JANA2_HAVE_PODIO
// Sadly, this will only work with C++17 or higher, and for now JANA still supports C++14 (when not using PODIO)

template <typename T>
struct PodioTypeMap;

template <typename, typename=void>
struct is_podio : std::false_type {};

template <typename T>
#if podio_VERSION >= PODIO_VERSION(0, 17, 0)
struct is_podio<T, std::void_t<typename T::collection_type>> : std::true_type {};
#else
/// The user was expected to provide some datamodel glue which specializes PodioTypeMap like this:
///    template <> struct PodioTypeMap<ExampleHit> {
///        using collection_t = ExampleHitCollection;
///        using mutable_t = MutableExampleHit;
///    }
struct is_podio<T, std::void_t<typename PodioTypeMap<T>::collection_t>> : std::true_type {};
#endif

template <typename T>
static constexpr bool is_podio_v = is_podio<T>::value;

#endif //if JANA2_HAVE_PODIO

