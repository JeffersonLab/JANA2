
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "TrackSmearingFactory.h"
#include "TrackMetadata.h"

#include <JANA/JEvent.h>

void TrackSmearingFactory::Init() {
}

void TrackSmearingFactory::ChangeRun(const std::shared_ptr<const JEvent> &event) {
}

void TrackSmearingFactory::Process(const std::shared_ptr<const JEvent> &event) {

    auto raw_tracks = event->Get<Track>("generated");
    std::vector<Track*> results;

    // TODO: Start timer

    // TODO: Do some work, with some delay
    // results.push_back(new Track(...));

    // TODO: Stop timer

    // Publish results and metadata
    JMetadata<Track> metadata;
    metadata.elapsed_time_ns = std::chrono::nanoseconds {10};
    SetMetadata(metadata);
    Set(results);
}
