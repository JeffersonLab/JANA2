
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "UserExceptionTests.h"
#include "catch.hpp"

#include <JANA/JApplication.h>

TEST_CASE("UserExceptionTests") {

    auto parms = new JParameterManager;
    //parms->SetParameter("log:debug","JApplication,JScheduler,JArrowProcessingController,JWorker,JArrow");
    JApplication app(parms);
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("JEventSource::Open() excepts, debug engine") {

        app.SetParameterValue("jana:engine", 1);
        app.Add(new FlakySource("open_excepting_source", &app, true, false));
        app.Add(new FlakyProcessor(&app, false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

   SECTION("JEventSource::GetEvent() excepts, debug engine") {

       app.SetParameterValue("jana:engine", 1);
       app.Add(new FlakySource("open_excepting_source", &app, false, true));
       app.Add(new FlakyProcessor(&app, false, false, false));
       REQUIRE_THROWS(app.Run(true));
   }


    SECTION("JEventProcessor::Init() excepts, debug engine") {

        app.SetParameterValue("jana:engine", 1);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, true, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

   SECTION("JEventProcessor::Process() excepts, debug engine") {

       app.SetParameterValue("jana:engine", 1);
       app.Add(new FlakySource("open_excepting_source", &app, false, false));
       app.Add(new FlakyProcessor(&app, false, true, false));
       REQUIRE_THROWS(app.Run(true));
   }

   SECTION("JEventProcessor::Finish() excepts, debug engine") {

       app.SetParameterValue("jana:engine", 1);
       app.Add(new FlakySource("open_excepting_source", &app, false, false));
       app.Add(new FlakyProcessor(&app, false, false, true));
       REQUIRE_THROWS(app.Run(true));
   }

    SECTION("JEventSource::Open() excepts, default engine") {

        app.SetParameterValue("jana:engine", 0);
        app.Add(new FlakySource("open_excepting_source", &app, true, false));
        app.Add(new FlakyProcessor(&app, false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventSource::GetEvent() excepts, default engine") {

        app.SetParameterValue("jana:engine", 0);
        app.Add(new FlakySource("open_excepting_source", &app, false, true));
        app.Add(new FlakyProcessor(&app, false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Init() excepts, default engine") {

        app.SetParameterValue("jana:engine", 0);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, true, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Process() excepts, default engine") {

        app.SetParameterValue("jana:engine", 0);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, false, true, false));
        REQUIRE_THROWS(app.Run(true));
    }

    SECTION("JEventProcessor::Finish() excepts, default engine") {

        app.SetParameterValue("jana:engine", 0);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, false, false, true));
        REQUIRE_THROWS(app.Run(true));
    }
}

