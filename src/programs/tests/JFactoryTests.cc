
#include "catch.hpp"
#include "JFactoryTests.h"

#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>

TEST_CASE("JFactoryTests") {


    SECTION("GetOrCreate calls Init, ChangeRun, and Process as needed") {

        DummyFactory sut;
        auto event = std::make_shared<JEvent>();

        // The first time it is run, Init, ChangeRun, and Process each get run once
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // We can getOrCreate as many times as we want and nothing will happen
        // until somebody clears the factory (assuming persistence flag is unset)
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // Once we clear the factory, Process gets called again but Init does not.
        // TODO: ChangeRun is doing the wrong thing.
        sut.ClearData();
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 2);
        REQUIRE(sut.process_call_count == 2);

    }

    // TODO: Does this belong here, or in JEventTests?
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


    SECTION("ChangeRun called only when run number changes") {
        auto event = std::make_shared<JEvent>();
        DummyFactory sut;

        // For the first event, ChangeRun() always gets called
        event->SetEventNumber(1);
        event->SetRunNumber(22);
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.change_run_call_count == 1);

        // Subsequent events with the same run number do not trigger ChangeRun()
        event->SetEventNumber(2);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr);

        event->SetEventNumber(3);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.change_run_call_count == 1);

        // As soon as the run number changes, ChangeRun() gets called again
        event->SetEventNumber(4);
        event->SetRunNumber(49);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.change_run_call_count == 2);

        // This keeps happening
        event->SetEventNumber(5);
        event->SetRunNumber(6180);
        sut.ClearData();
        sut.GetOrCreate(event, nullptr);
        REQUIRE(sut.change_run_call_count == 3);
    }

    SECTION("Persistent flag determines whether data gets cleared on every event or not") {
        // This is true regardless of how the data got there.
        DummyFactory sut;
        bool deleted_flag = false;

        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        auto result = sut.GetOrCreate(nullptr, nullptr);
        //REQUIRE(result.first->data == 42);
        //REQUIRE(deleted_flag == true);

        deleted_flag = false;
        sut.SetFactoryFlag(JFactory::PERSISTANT);
        sut.Insert(new DummyObject(42, &deleted_flag));
        sut.ClearData();
        result = sut.GetOrCreate(nullptr, nullptr);
        //REQUIRE(result.first == result.second);
        //REQUIRE(deleted_flag == false);

    }

    SECTION("NOT_OBJECT_OWNER flag determines whether data gets deleted whenever it gets cleared") {

    }
}

