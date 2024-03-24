// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/JEventSource.h>


struct MyTimesliceSource : public JEventSource {

    PodioOutput<ExampleHit> m_hits_out {this, "hits"};

    MyTimesliceSource() {
        SetLevel(JEventLevel::Timeslice);
        SetTypeName("MyTimesliceSource");
    }

    void Open() override { }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto ts_nr = event->GetEventNumber();
        auto hits_out  = std::make_unique<ExampleHitCollection>();
        hits_out->push_back(ExampleHit(0, 0, 0, 0, ts_nr, 0));

        std::ostringstream oss;
        oss << "----------------------" << std::endl;
        oss << "MyTimesliceSource: Timeslice " << event->GetEventNumber() << std::endl;
        hits_out->print(oss);
        oss << "----------------------" << std::endl;
        LOG << oss.str() << LOG_END;

        m_hits_out() = std::move(hits_out);
    }
};
