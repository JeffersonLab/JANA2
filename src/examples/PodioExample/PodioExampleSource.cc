
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "PodioExampleSource.h"

std::unique_ptr<podio::Frame> PodioExampleSource::NextFrame(int event_index, int &event_number, int &run_number) {

    auto frame_data = m_reader.readEntry("events", event_index);
    auto frame = std::make_unique<podio::Frame>(std::move(frame_data));

    // TODO: Obtain event and run numbers
    event_number = 22;
    run_number = 44;
    return frame;
}
