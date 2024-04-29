
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "TerminationTests.h"
#include "catch.hpp"
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/JApplication.h>
#include <thread>



TEST_CASE("TerminationTests") {

    JApplication app;
    app.SetParameterValue("log:global", "OFF");
    auto processor = new CountingProcessor();
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("Manual termination") {

        auto source = new UnboundedSource();
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
    }

    SECTION("Self termination") {

        auto source = new BoundedSource();
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
    }

    SECTION("Interrupted during JEventSource::Open()") {

        auto source = new InterruptedSource();
        app.Add(source);
        app.Run(true);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->GetEventCount());
        // We don't know how many events will emit before Stop() request propagates
    }

};


