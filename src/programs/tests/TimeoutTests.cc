
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "TimeoutTests.h"
#include "catch.hpp"

#include <JANA/JApplication.h>

TEST_CASE("TimeoutTests", "[.][performance") {

    std::cout << "Running timeout tests..." << std::endl;
    JApplication app;
    app.SetTimeoutEnabled(true);
    app.SetTicker(true);
    app.SetParameterValue("jana:timeout", 1);
    app.SetParameterValue("jana:warmup_timeout", 3);


    SECTION("Timeout in the event source on the first event") {
        app.Add(new SourceWithTimeout("source_with_timeout", &app, 1));
        app.Add(new ProcessorWithTimeout(-1));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Timeout);
    }

    SECTION("Timeout in the event source on the 5th event") {
        app.Add(new SourceWithTimeout("source_with_timeout", &app, 5));
        app.Add(new ProcessorWithTimeout(-1));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Timeout);
    }

    SECTION("Timeout in the event processor on the first event") {
        app.Add(new SourceWithTimeout("source_with_timeout", &app, -1));
        app.Add(new ProcessorWithTimeout(1));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Timeout);
    }

    SECTION("Timeout in the event processor on the 5th event") {
        app.Add(new SourceWithTimeout("source_with_timeout", &app, -1));
        app.Add(new ProcessorWithTimeout(5));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Timeout);
    }

    SECTION("A slow event source for the first event is fine") {

        int first_event_ms = 2000;
        app.Add(new SourceWithTimeout("source_with_timeout", &app, -1, first_event_ms));
        app.Add(new ProcessorWithTimeout(-1, 0));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Success);
    }

    SECTION("A slow event processor for the first event is fine") {

        int first_event_ms = 2000;
        app.Add(new SourceWithTimeout("source_with_timeout", &app, -1, 0));
        app.Add(new ProcessorWithTimeout(-1, first_event_ms));
        app.Run(true);
        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Success);
    }


}

