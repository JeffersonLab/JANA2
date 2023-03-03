
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JPODIOTYPEHELPERS_H
#define JANA2_JPODIOTYPEHELPERS_H

#include <type_traits>

/// These allow us to have both a PODIO-enabled and a PODIO-free definition of certain key structures and functions.
/// Ideally, we would use concepts for this. However, we are limiting ourselves to C++17 for now.
/// The user is expected to provide some datamodel glue which specializes PodioTypeMap like this:
///    template <> struct PodioTypeMap<ExampleHit> {
///        using collection_t = ExampleHitCollection;
///        using mutable_t = MutableExampleHit;
///    }
/// Eventually we hope to hang these type relations off of the Podio types themselves.

#ifdef HAVE_PODIO
// Sadly, this will only work with C++17 or higher, and for now JANA still supports C++14 (when not using PODIO)

template <typename T>
struct PodioTypeMap;

template <typename, typename=void>
struct is_podio : std::false_type {};

template <typename T>
struct is_podio<T, std::void_t<typename PodioTypeMap<T>::collection_t>> : std::true_type {};

template <typename T>
static constexpr bool is_podio_v = is_podio<T>::value;

#endif //ifdef HAVE_PODIO

#endif //JANA2_JPODIOTYPEHELPERS_H
