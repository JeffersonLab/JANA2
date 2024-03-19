
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "TerminationTests.h"
#include "catch.hpp"
#include "JANA/Engine/JArrowProcessingController.h"
#include <thread>

#include <JANA/JApplication.h>
#include <JANA/Engine/JArrowTopology.h>



TEST_CASE("TerminationTests") {

    auto parms = new JParameterManager;
    // parms->SetParameter("log:debug","JScheduler,JArrowProcessingController,JWorker,JArrow");
    JApplication app(parms);
    auto processor = new CountingProcessor();
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("Arrow engine, manual termination") {

        app.SetParameterValue("jana:engine", 0);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
    }

    SECTION("Arrow engine, self termination") {

        app.SetParameterValue("jana:engine", 0);
        auto source = new BoundedSource("BoundedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
    }

    SECTION("Arrow engine, interrupted during JEventSource::Open()") {

        // TODO: This test is kind of useless now that JEventSource::Open is called from
        //       JEventSourceArrow::execute rather than JEventSourceArrow::initialize().
        //       What we really want is an Arrow that has an initialize() that we override.
        //       However to do that, we need to extend JESA and create a custom topology.
        app.SetParameterValue("jana:engine", 0);
        auto source = new InterruptedSource("InterruptedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(processor->processed_count == 1);  // TODO: Was 0, should become zero again
        REQUIRE(processor->finish_call_count == 1);
        // Stop() tells JApplication to finish Initialize() but not to proceed with Run().
        // If we had called Quit() instead, it would have exited Initialize() immediately and ended the program.

        REQUIRE(app.GetNEventsProcessed() == source->GetEventCount());
    }

    SECTION("Debug engine, self-termination") {

        app.SetParameterValue("jana:engine", 1);
        auto source = new BoundedSource("BoundedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
        REQUIRE(processor->finish_call_count == 1);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
    }

    SECTION("Debug engine, manual termination") {

        app.SetParameterValue("jana:engine", 1);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
        REQUIRE(app.GetNEventsProcessed() == source->event_count);
        REQUIRE(processor->finish_call_count == 1);
    }

};


