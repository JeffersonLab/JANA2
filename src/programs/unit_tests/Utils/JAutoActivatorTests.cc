
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JANA/Components/JComponentFwd.h"
#include "catch.hpp"
#include <JANA/Utils/JAutoActivator.h>
#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JFactoryT.h>
#include <JANA/JEventProcessor.h>



TEST_CASE("Autoactivate_Split") {
    auto pair = JAutoActivator::Split("all_just_objname");
    REQUIRE(pair.first == "all_just_objname");
    REQUIRE(pair.second == "");

    pair = JAutoActivator::Split("objname:tag");
    REQUIRE(pair.first == "objname");
    REQUIRE(pair.second == "tag");

    pair = JAutoActivator::Split("name::type:factag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "factag");

    pair = JAutoActivator::Split("name::type");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "");

    pair = JAutoActivator::Split("name::type:tag:more_tag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "tag:more_tag");

    pair = JAutoActivator::Split("name::type:tag:more_tag::most_tag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "tag:more_tag::most_tag");

    pair = JAutoActivator::Split("::name::type:tag:more_tag::most_tag");
    REQUIRE(pair.first == "::name::type");
    REQUIRE(pair.second == "tag:more_tag::most_tag");

    pair = JAutoActivator::Split(":I_guess_this_should_be_a_tag_idk");
    REQUIRE(pair.first == "");
    REQUIRE(pair.second == "I_guess_this_should_be_a_tag_idk");
}

namespace jana::autoactivate_ordering_tests {

struct TestData {int x = 22;};

class TestFac : public JFactoryT<TestData> {
    void Process(const std::shared_ptr<const JEvent>&) override {
        LOG_INFO(GetLogger()) << "Running TestFac";
        Insert(new TestData);
    }
};

struct TestProcLegacy : public JEventProcessor {
    TestProcLegacy() {
        SetTypeName("TestProcLegacy");
        SetCallbackStyle(CallbackStyle::LegacyMode);
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        LOG_INFO(GetLogger()) << "Running TestProcLegacy";
        auto fac = event->GetFactory<TestData>();
        REQUIRE(fac->GetStatus() == JFactory::Status::Processed);
    }
};

struct TestProcExpert : public JEventProcessor {
    TestProcExpert() {
        SetTypeName("TestProcExpert");
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Process(const JEvent& event) override {
        LOG_INFO(GetLogger()) << "Running TestProcExpert";
        auto fac = event.GetFactory<TestData>();
        REQUIRE(fac->GetStatus() == JFactory::Status::Processed);
    }
};


TEST_CASE("Autoactivate_Ordering") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 1);
    app.SetParameterValue("jana:loglevel", "debug");
    app.SetParameterValue("autoactivate", "jana::autoactivate_ordering_tests::TestData");
    app.Add(new JEventSource);
    app.Add(new JFactoryGeneratorT<TestFac>);

    SECTION("LegacyMode") {
        app.Add(new TestProcLegacy);
        app.Run();
    }
    SECTION("ExpertMode") {
        app.Add(new TestProcExpert);
        app.Run();
    }
    SECTION("LegacyAndExpertMode") {
        app.Add(new TestProcLegacy);
        app.Add(new TestProcExpert);
        app.Run();
    }
}


} // namespace
