
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "UserExceptionTests.h"
#include "catch.hpp"

#include <JANA/JApplication.h>

TEST_CASE("UserExceptionTests") {

    JApplication app;
    app.SetParameterValue("log:debug","JApplication,JScheduler,JArrowProcessingController,JWorker,JArrow");
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("JEventSource::Open() excepts") {

        app.Add(new FlakySource(true, false));
        app.Add(new FlakyProcessor(false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventSource::GetEvent() excepts") {

        app.Add(new FlakySource(false, true));
        app.Add(new FlakyProcessor(false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Init() excepts") {

        app.Add(new FlakySource(false, false));
        app.Add(new FlakyProcessor(true, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Process() excepts") {

        app.Add(new FlakySource(false, false));
        app.Add(new FlakyProcessor(false, true, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Finish() excepts") {

        app.Add(new FlakySource(false, false));
        app.Add(new FlakyProcessor(false, false, true));
        REQUIRE_THROWS(app.Run(true));
    }
}

