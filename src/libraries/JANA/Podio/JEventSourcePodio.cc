
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JEventSourcePodio.h"
#include "PodioFrame.h"

JEventSourcePodio::JEventSourcePodio(std::string filename)
: JEventSource(filename) {
}

void JEventSourcePodio::GetEvent(std::shared_ptr<JEvent> event) {
    auto frame = NextFrame(event->GetEventNumber());
    // TODO: Each collection in the frame needs to be inserted into a JFactory
    // This is going to require some additional datamodel glue
    /*
    for (auto collection : frame) {

    }
    */
    event->Insert<PodioFrame>(frame.release());
}

void JEventSourcePodio::Open() {
    // TODO: Open ROOT file and fail if necessary
}

void JEventSourcePodio::Close() {
    // TODO: Close ROOT file
}


