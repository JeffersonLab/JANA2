
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JEventProcessorPodio.h"
#include "PodioFrame.h"

void JEventProcessorPodio::Init() {
    // TODO: Test that output file is writable and fail otherwise
}

void JEventProcessorPodio::Process(const std::shared_ptr<const JEvent> &event) {
    auto frame = event->Get<PodioFrame>();
    // TODO: Persist frame to output file
}

void JEventProcessorPodio::Finish() {
    // TODO: Close file
}
