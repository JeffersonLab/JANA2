//
// Created by Nathan Brei on 2019-07-19.
//

#include "TerminationTests.h"
#include "catch.hpp"
#include <thread>

#include <JANA/JApplication.h>

TEST_CASE("TerminationTests") {

    JApplication app;
    japp = &app;
    auto processor = new CountingProcessor(&app);
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("Old engine, self-termination") {

        app.SetParameterValue("jana:legacy_mode", 1);
        auto source = new BoundedSource("BoundedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
    }

    SECTION("New engine, self-termination") {

        app.SetParameterValue("jana:legacy_mode", 0);
        auto source = new BoundedSource("BoundedSource", &app);
        app.Add(source);
        app.Run(true);
        REQUIRE(source->event_count == 10);
        REQUIRE(processor->processed_count == 10);
    }

    SECTION("Old engine, manual termination") {

        app.SetParameterValue("jana:legacy_mode", 1);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
    }

    SECTION("New engine, manual termination") {

        app.SetParameterValue("jana:legacy_mode", 0);
        auto source = new UnboundedSource("UnboundedSource", &app);
        app.Add(source);
        app.Run(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        app.Stop(true);
        REQUIRE(source->event_count > 0);
    }
};




