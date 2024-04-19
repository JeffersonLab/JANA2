// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/JEventSource.h>
#include "CollectionTabulators.h"


struct MyFileReader : public JEventSource {

    PodioOutput<ExampleHit> m_hits_out {this, "hits"};

    MyFileReader() {
        SetTypeName(NAME_OF_THIS);
    }

    void Open() override { }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto event_nr = event->GetEventNumber();

        auto hits_out  = std::make_unique<ExampleHitCollection>();

        // ExampleHit(unsigned long long cellID, double x, double y, double z, double energy, std::uint64_t time);
        hits_out->push_back(ExampleHit(event_nr, 0, 22, 22, 22, 0));
        hits_out->push_back(ExampleHit(event_nr, 0, 49, 49, 49, 1));
        hits_out->push_back(ExampleHit(event_nr, 0, 7.6, 7.6, 7.6, 2));

        LOG_DEBUG(GetLogger()) << "MySource: Emitted " << GetLevel() << " " << event->GetEventNumber() << "\n"
            << TabulateHits(hits_out.get())
            << LOG_END;

        m_hits_out() = std::move(hits_out);

        // Furnish this event with info object
        if (GetLevel() == JEventLevel::Timeslice) {
            TimesliceInfoCollection info;
            info.push_back(TimesliceInfo(event_nr, 0)); // event nr, run nr
            event->InsertCollection<TimesliceInfo>(std::move(info), "ts_info");
        }
        else {
            EventInfoCollection info;
            info.push_back(EventInfo(event_nr, 0, 0)); // event nr, timeslice nr, run nr
            event->InsertCollection<EventInfo>(std::move(info), "evt_info");
        }
    }
};
