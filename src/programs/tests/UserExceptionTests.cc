//
// Created by Nathan Brei on 2019-07-19.
//

#include "UserExceptionTests.h"
#include "catch.hpp"

#include <JANA/JApplication.h>

TEST_CASE("UserExceptionTests") {

    JApplication app;
    app.SetParameterValue("jana:extended_report", 0);

    SECTION("JEventSource::Open() excepts, old engine") {

        app.SetParameterValue("jana:legacy_mode", 1);
        app.Add(new FlakySource("open_excepting_source", &app, true, false));
        app.Add(new FlakyProcessor(&app, false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

    // TODO: Re-enable once we forward exceptions to master thread
//    SECTION("JEventSource::GetEvent() excepts, old engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 1);
//        app.Add(new FlakySource("open_excepting_source", &app, false, true));
//        app.Add(new FlakyProcessor(&app, false, false, false));
//        //REQUIRE_THROWS(app.Run(true));
//        app.Run(true);
//        REQUIRE(app.GetExitCode() == 99);
//    }

    SECTION("JEventProcessor::Init() excepts, old engine") {

        app.SetParameterValue("jana:legacy_mode", 1);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, true, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

//    SECTION("JAbstractEventProcessor::Process() excepts, old engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 1);
//        app.Add(new FlakySource("open_excepting_source", &app, false, false));
//        app.Add(new FlakyProcessor(&app, false, true, false));
//        REQUIRE_THROWS(app.Run(true));
//    }

//    SECTION("JEventSource::Finish() excepts, old engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 1);
//        app.Add(new FlakySource("open_excepting_source", &app, false, false));
//        app.Add(new FlakyProcessor(&app, false, false, true));
//        REQUIRE_THROWS(app.Run(true));
//    }

    SECTION("JEventSource::Open() excepts, new engine") {

        app.SetParameterValue("jana:legacy_mode", 0);
        app.Add(new FlakySource("open_excepting_source", &app, true, false));
        app.Add(new FlakyProcessor(&app, false, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

//    SECTION("JEventSource::GetEvent() excepts, new engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 0);
//        app.Add(new FlakySource("open_excepting_source", &app, false, true));
//        app.Add(new FlakyProcessor(&app, false, false, false));
//        REQUIRE_THROWS(app.Run(true));
//    }

    SECTION("JEventProcessor::Init() excepts, new engine") {

        app.SetParameterValue("jana:legacy_mode", 0);
        app.Add(new FlakySource("open_excepting_source", &app, false, false));
        app.Add(new FlakyProcessor(&app, true, false, false));
        REQUIRE_THROWS(app.Run(true));
    }

//    SECTION("JAbstractEventProcessor::Process() excepts, new engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 0);
//        app.Add(new FlakySource("open_excepting_source", &app, false, false));
//        app.Add(new FlakyProcessor(&app, false, true, false));
//        REQUIRE_THROWS(app.Run(true));
//    }

//    SECTION("JAbstractEventProcessor::Finish() excepts, new engine") {
//
//        app.SetParameterValue("jana:legacy_mode", 0);
//        app.Add(new FlakySource("open_excepting_source", &app, false, false));
//        app.Add(new FlakyProcessor(&app, false, false, true));
//        REQUIRE_THROWS(app.Run(true));
//    }
}

