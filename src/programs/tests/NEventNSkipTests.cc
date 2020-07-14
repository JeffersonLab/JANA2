
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/JEventSource.h>


struct NEventNSkipBoundedSource : public JEventSource {

    std::atomic_int event_count {0};
    int event_bound = 100;
    std::vector<int> events_emitted;

    NEventNSkipBoundedSource(std::string source_name, JApplication *app) : JEventSource(source_name, app) { }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        if (event_count >= event_bound) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        event_count += 1;
        events_emitted.push_back(event_count);
    }
};


TEST_CASE("NEventNSkipTests") {

    JApplication app;
    auto source = new NEventNSkipBoundedSource("BoundedSource", &app);
    app.Add(source);

    SECTION("[1..100] @ nskip=0, nevents=0 => [1..100]") {

        app.SetParameterValue("jana:nskip", 0);
        app.SetParameterValue("jana:nevents", 0);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 100);
        REQUIRE(source->events_emitted.size() == 100);
        REQUIRE(source->events_emitted[0] == 1);
        REQUIRE(source->events_emitted[99] == 100);
    }

    SECTION("[1..100] @ nskip=0, nevents=22 => [1..22]") {

        app.SetParameterValue("jana:nskip", 0);
        app.SetParameterValue("jana:nevents", 22);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 22);
        REQUIRE(source->events_emitted.size() == 22);
        REQUIRE(source->events_emitted[0] == 1);
        REQUIRE(source->events_emitted[21] == 22);
    }

    SECTION("[1..100] @ nskip=30, nevents=20 => [31..51]") {

        app.SetParameterValue("jana:nskip", 30);
        app.SetParameterValue("jana:nevents", 20);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 50);
        // All 50 events get filled, but the first 30 are discarded without emitting
        // However, all 50 go into events_emitted
    }
}

