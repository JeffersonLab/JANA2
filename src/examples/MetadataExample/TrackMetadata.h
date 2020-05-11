//
// Created by Nathan Brei on 5/11/20.
//

#ifndef JANA2_TRACKMETADATA_H
#define JANA2_TRACKMETADATA_H

#include "Track.h"
#include <chrono>

/// We use the JMetadata template trait in order to attach arbitrary metadata to any JFactory<T>.
/// This way we can extract metadata from our JFactory<T> while only knowing T. In other words,
/// we are free to swap in and out different JFactories and still retrieve the metadata in our JEventProcessors.

/// In this example, any JFactory which produces Tracks can also produced a time duration indicating how long
/// the operation took.
template <>
struct JMetadata<Track> {

    std::chrono::nanoseconds elapsed_time_ns = 0;
};

#endif //JANA2_TRACKMETADATA_H
