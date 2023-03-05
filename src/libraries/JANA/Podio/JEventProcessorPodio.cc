
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JEventProcessorPodio.h"

void JEventProcessorPodio::Init() {
    // TODO: Obtain m_output_filename, etc, from parameter manager
    // TODO: Does PODIO test that output file is writable and fail otherwise?
    //       We want to throw an exception immediately so that we don't waste compute time

    m_writer = std::make_unique<podio::ROOTFrameWriter>(m_output_filename);
}

void JEventProcessorPodio::Process(const std::shared_ptr<const JEvent> &event) {

    auto* frame = event->GetSingle<podio::Frame>();
    // This will throw if no PODIO frame is found. There will be no PODIO frame if the event source doesn't insert any
    // PODIO classes, or there are no JFactoryPodioT's provided.
    // Is this really the behavior we want? The alternatives are to silently not write anything, or to print a warning.

    m_writer->writeFrame(*frame, "events");
    // Note: This won't include event/run number unless somebody added it explicitly,
    //       presumably in the event source. We may find ourselves revisiting this
}

void JEventProcessorPodio::Finish() {
    m_writer->finish();
}
