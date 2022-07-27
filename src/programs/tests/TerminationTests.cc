
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "TerminationTests.h"
#include "catch.hpp"
#include "JANA/Engine/JArrowProcessingController.h"
#include <thread>

#include <JANA/JApplication.h>
#include <JANA/Engine/JArrowTopology.h>



TEST_CASE("TerminationTests") {

    JApplication app;
    auto processor = new CountingProcessor(&app);
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("Arrow engine, manual termination") {

        app.SetParameterValue("jana:engine", 0);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
    }

    SECTION("Debug engine, self-termination") {

        app.SetParameterValue("jana:engine", 1);
        auto source = new BoundedSource("BoundedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
    }

    SECTION("Debug engine, manual termination") {

        app.SetParameterValue("jana:engine", 1);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
    }

};



TEST_CASE("TerminationTestsIssue119") {


    auto parms = new JParameterManager;
    // parms->SetParameter("log:debug","JScheduler,JArrowProcessingController,JWorker,JArrow");

    JApplication app(parms);
    auto processor = new CountingProcessor(&app);
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 1);
    app.SetParameterValue("jana:engine", 0);
    app.SetParameterValue("nthreads", 4);
    app.SetTicker(true);

    auto source = new UnboundedSource("UnboundedSource", &app);
    app.Add(source);
    app.Run(false);
    for (int secs = 0; secs < 2; secs++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // app.PrintStatus();
    }
    app.Quit(false);
    REQUIRE(source->event_count > 0);
    REQUIRE(processor->finish_call_count == 1);
}



TEST_CASE("SelfTermination") {

    auto parms = new JParameterManager;
    parms->SetParameter("log:debug","JScheduler,JArrowProcessingController,JWorker,JArrow");
    JApplication app(parms);
    auto processor = new CountingProcessor(&app);
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    app.SetParameterValue("jana:engine", 0);
    auto source = new BoundedSource("BoundedSource", &app);
    app.Add(source);
    app.Run(true);
    REQUIRE(source->event_count == 10);
    REQUIRE(processor->processed_count == 10);
}




