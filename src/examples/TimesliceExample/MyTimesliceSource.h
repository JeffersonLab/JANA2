// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/JEventSource.h>


struct MyTimesliceSource : public JEventSource {

    PodioOutput<ExampleHit> hits_out {this, "hits"};

    MyTimesliceSource() {
        SetLevel(JEventLevel::Timeslice);
        SetTypeName("MyTimesliceSource");
    }

    void Open() override { }

    void GetEvent(std::shared_ptr<JEvent> /*event*/) override {

        /*
        auto evt = event->GetEventNumber();
        auto hits = std::make_unique<ExampleHitCollection>();
        hits.push_back(ExampleHit(22));
        hits.push_back(ExampleHit(23));
        hits.push_back(ExampleHit(24));
        event->InsertCollection(hits);
        */
    }
};
