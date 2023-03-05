
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "PodioExampleSource.h"
#include <datamodel/EventInfo.h>

std::unique_ptr<podio::Frame> PodioExampleSource::NextFrame(int event_index, int &event_number, int &run_number) {

    auto frame_data = m_reader.readEntry("events", event_index);
    auto frame = std::make_unique<podio::Frame>(std::move(frame_data));
    auto& eventinfo = frame->get<EventInfoCollection>("eventinfos");
    if (eventinfo.size() != 1) throw JException("Bad eventinfo: Entry %d contains %d items, 1 expected.", event_index, eventinfo.size());

    event_number = eventinfo[0].EventNumber();
    run_number = eventinfo[0].RunNumber();
    return frame;
}
