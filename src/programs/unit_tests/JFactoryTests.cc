
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"
#include "JFactoryTests.h"

#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>

TEST_CASE("JFactoryTests") {


    SECTION("CreateAndGetData calls Init, ChangeRun, and Process as needed") {

        JFactoryTestDummyFactory sut;
        auto event = std::make_shared<JEvent>();

        // The first time it is run, Init, ChangeRun, and Process each get run once
        sut.CreateAndGetData(event);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // We can getOrCreate as many times as we want and nothing will happen
        // until somebody clears the factory (assuming persistence flag is unset)
        sut.CreateAndGetData(event);
        REQUIRE(sut.init_call_count == 1);
        REQUIRE(sut.change_run_call_count == 1);
        REQUIRE(sut.process_call_count == 1);

        // Once we clear the factory, Process gets called again but Init and ChangeRun do not.
        sut.ClearData();
        sut.CreateAndGetData(event);
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
        JFactoryTestDummyFactory sut;

        // For the first event, ChangeRun() always gets called
        event->SetEventNumber(1);
        event->SetRunNumber(22);
        sut.CreateAndGetData(event);
        REQUIRE(sut.change_run_call_count == 1);

        // Subsequent events with the same run number do not trigger ChangeRun()
        event->SetEventNumber(2);
        sut.ClearData();
        sut.CreateAndGetData(event);

        event->SetEventNumber(3);
        sut.ClearData();
        sut.CreateAndGetData(event);
        REQUIRE(sut.change_run_call_count == 1);

        // As soon as the run number changes, ChangeRun() gets called again
        event->SetEventNumber(4);
        event->SetRunNumber(49);
        sut.ClearData();
        sut.CreateAndGetData(event);
        REQUIRE(sut.change_run_call_count == 2);

        // This keeps happening
        event->SetEventNumber(5);
        event->SetRunNumber(6180);
        sut.ClearData();
        sut.CreateAndGetData(event);
        REQUIRE(sut.change_run_call_count == 3);
    }

    SECTION("not PERSISTENT && not NOT_OBJECT_OWNER => JObject is cleared and deleted") {
        auto event = std::make_shared<JEvent>();
        JFactoryT<JFactoryTestDummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.ClearFactoryFlag(JFactory::PERSISTENT);
        sut.ClearFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new JFactoryTestDummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.CreateAndGetData(event);
        REQUIRE(std::distance(results.first, results.second) == 0);
        REQUIRE(deleted_flag == true);
    }

    SECTION("not PERSISTENT && NOT_OBJECT_OWNER => JObject is cleared but not deleted") {
        auto event = std::make_shared<JEvent>();
        JFactoryT<JFactoryTestDummyObject> sut; // Process() is a no-op
        bool deleted_flag = false;
        sut.ClearFactoryFlag(JFactory::PERSISTENT);
        sut.SetFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new JFactoryTestDummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.CreateAndGetData(event);
        REQUIRE(std::distance(results.first, results.second) == 0);
        REQUIRE(deleted_flag == false);
    }

    SECTION("PERSISTENT && not NOT_OBJECT_OWNER => JObject is neither cleared nor deleted") {
        JFactoryT<JFactoryTestDummyObject> sut; // Process() is a no-op
        auto event = std::make_shared<JEvent>();
        bool deleted_flag = false;
        sut.SetFactoryFlag(JFactory::PERSISTENT);
        sut.ClearFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new JFactoryTestDummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.CreateAndGetData(event);
        REQUIRE(std::distance(results.first, results.second) == 1);
        REQUIRE(deleted_flag == false);
    }

    SECTION("PERSISTENT && NOT_OBJECT_OWNER => JObject is neither cleared nor deleted") {
        JFactoryT<JFactoryTestDummyObject> sut; // Process() is a no-op
        auto event = std::make_shared<JEvent>();
        bool deleted_flag = false;
        sut.SetFactoryFlag(JFactory::PERSISTENT);
        sut.SetFactoryFlag(JFactory::NOT_OBJECT_OWNER);
        sut.Insert(new JFactoryTestDummyObject(42, &deleted_flag));
        sut.ClearData();
        auto results = sut.CreateAndGetData(event);
        REQUIRE(std::distance(results.first, results.second) == 1);
        REQUIRE(deleted_flag == false);
    }

    struct Issue135Factory : public JFactoryT<JFactoryTestDummyObject> {
        void Process(const std::shared_ptr<const JEvent>&) override {
            mData.emplace_back(new JFactoryTestDummyObject(3));
            mData.emplace_back(new JFactoryTestDummyObject(4));
            mData.emplace_back(new JFactoryTestDummyObject(5));
            Set(mData);
        }
    };
    SECTION("Issue 135: Users modifying mData directly and calling Set() afterwards") {
        // Unfortunately, we are giving users the ability to access mData directly.
        // If they do this, they might think they should call Set() directly afterwards so that
        // the Status and CreationStatus flags get correctly set.
        // Previously, this would clear mData before setting it, which would make their data
        // disappear, and they'd have to dig deep into the JANA internals to find out why.
        // Now, we account for this case. I still think we should clean up this abstraction, though.

        Issue135Factory sut;
        auto event = std::make_shared<JEvent>();
        auto results = sut.CreateAndGetData(event);
        REQUIRE(sut.GetNumObjects() == 3);
        REQUIRE(std::distance(results.first, results.second) == 3);

        int data = 3;
        for (auto it = results.first; it != results.second; ++it ) {
            REQUIRE((*it)->data == data);
            data++;
        }
    }

    struct RegenerateFactory : public JFactoryT<JFactoryTestDummyObject> {
        RegenerateFactory() {
            SetRegenerateFlag(true);
        }
        void Process(const std::shared_ptr<const JEvent>&) override {
            mData.emplace_back(new JFactoryTestDummyObject(49));
            Set(mData);
        }
    };

    SECTION("Factory regeneration") {
        RegenerateFactory sut;
        auto event = std::make_shared<JEvent>();

        std::vector<JFactoryTestDummyObject*> inserted_data;
        inserted_data.push_back(new JFactoryTestDummyObject(22));
        inserted_data.push_back(new JFactoryTestDummyObject(618));
        sut.Set(inserted_data);

        REQUIRE(sut.GetStatus() == JFactory::Status::Inserted);

        auto results = sut.CreateAndGetData(event);
        auto it = results.first;
        REQUIRE((*it)->data == 49);
        REQUIRE(sut.GetNumObjects() == 1);
    }
}

