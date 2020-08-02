
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"
#include "JFactoryTests.h"

#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>

TEST_CASE("JFactoryTests") {


    SECTION("GetOrCreate calls Init, ChangeRun, and Process as needed") {

        DummyFactory sut;
        auto event = std::make_shared<JEvent>();

        // The first time it is run, Init, ChangeRun, and Process each get run once
        sut.GetOrCreate(event, nullptr, 0);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // We can getOrCreate as many times as we want and nothing will happen
        // until somebody clears the factory (assuming persistence flag is unset)
        sut.GetOrCreate(event, nullptr, 0);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // Once we clear the factory, Process gets called again but Init and ChangeRun do not.
        sut.ClearData();
        sut.GetOrCreate(event, nullptr, 0);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 2);

    }

/*
    SECTION("If no factory is present and nothing inserted, GetObjects called") {

        // The event hasn't been given any DummyFactory, nor has anything been inserted.
        // Instead, the JEvent knows to go down to the JEventSource.
        auto event = std::make_shared<JEvent>();
        DummySource sut;
        event->SetJEventSource(&sut);
        event->SetFactorySet(new JFactorySet());

        auto data = event->Get<DummyObject>("");
        REQUIRE(data[0]->data == 8);
        REQUIRE(data[1]->data == 88);
    }

*/
    SECTION("ChangeRun called only when run number changes") {
        auto event = std::make_shared<JEvent>();
        DummyFactory sut;

        // For the first event, ChangeRun() always gets called
        event->SetEventNumber(1);
        event->SetRunNumber(22);
        sut.GetOrCreate(event, nullptr, 22);
        REQUIRE(sut.change_run_call_count == 1);

        // Subsequent events with the same run number do not trigger ChangeRun()
        event->SetEventNumber(2);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr, 22);

        event->SetEventNumber(3);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr, 22);
        REQUIRE(sut.change_run_call_count == 1);

        // As soon as the run number changes, ChangeRun() gets called again
        event->SetEventNumber(4);
        event->SetRunNumber(49);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr, 49);
        REQUIRE(sut.change_run_call_count == 2);

        // This keeps happening
        event->SetEventNumber(5);
        event->SetRunNumber(6180);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr, 6180);
        REQUIRE(sut.change_run_call_count == 3);
    }

    SECTION("not PERSISTENT && not NOT_OBJECT_OWNER => JObject is cleared and deleted") {
        JFactoryT<DummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.ClearFactoryFlag(JFactory::PERSISTENT);
        sut.ClearFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.GetOrCreate(nullptr, nullptr, 0);
        REQUIRE(std::distance(results.first, results.second) == 0);
        REQUIRE(deleted_flag == true);
    }

    SECTION("not PERSISTENT && NOT_OBJECT_OWNER => JObject is cleared but not deleted") {
        JFactoryT<DummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.ClearFactoryFlag(JFactory::PERSISTENT);
        sut.SetFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.GetOrCreate(nullptr, nullptr, 0);
        REQUIRE(std::distance(results.first, results.second) == 0);
        REQUIRE(deleted_flag == false);
    }

    SECTION("PERSISTENT && not NOT_OBJECT_OWNER => JObject is neither cleared nor deleted") {
        JFactoryT<DummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.SetFactoryFlag(JFactory::PERSISTENT);
        sut.ClearFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.GetOrCreate(nullptr, nullptr, 0);
        REQUIRE(std::distance(results.first, results.second) == 1);
        REQUIRE(deleted_flag == false);
    }

    SECTION("PERSISTENT && NOT_OBJECT_OWNER => JObject is neither cleared nor deleted") {
        JFactoryT<DummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.SetFactoryFlag(JFactory::PERSISTENT);
        sut.SetFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.GetOrCreate(nullptr, nullptr, 0);
        REQUIRE(std::distance(results.first, results.second) == 1);
        REQUIRE(deleted_flag == false);
    }
}

