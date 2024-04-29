
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Services/JEventGroupTracker.h>

#include "catch.hpp"

TEST_CASE("JEventGroupTests") {

    JEventGroupManager manager;

    SECTION("Happy path") {
        auto sut = manager.GetEventGroup(22);
        REQUIRE(sut->IsGroupFinished() == true);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->CloseGroup();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->FinishEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->FinishEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->FinishEvent();
        REQUIRE(sut->IsGroupFinished() == true);
    }

    SECTION("Groups are initially closed until an event is started") {
        auto sut = manager.GetEventGroup(22);
        REQUIRE(sut->IsGroupFinished() == true);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->CloseGroup();
        sut->FinishEvent();
        REQUIRE(sut->IsGroupFinished() == true);
    }

    SECTION("Pointer equality <=> group id equality") {
        auto group1 = manager.GetEventGroup(22);
        auto group2 = manager.GetEventGroup(22);
        REQUIRE(group1 == group2);
        auto group3 = manager.GetEventGroup(23);
        REQUIRE(group1 != group3);
    }

    SECTION("Reusing groups works the way you'd expect") {
        auto sut = manager.GetEventGroup(22);
        REQUIRE(sut->IsGroupFinished() == true);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->FinishEvent();
        sut->CloseGroup();
        REQUIRE(sut->IsGroupFinished() == true);
        sut->StartEvent();
        REQUIRE(sut->IsGroupFinished() == false);
        sut->FinishEvent();
        sut->CloseGroup();
        REQUIRE(sut->IsGroupFinished() == true);
    }

}