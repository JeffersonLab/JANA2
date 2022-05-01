
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>
#include <JANA/Utils/JCallGraphRecorder.h>
#include "JANA/JEvent.h"

TEST_CASE("Test topological sort algorithm in isolation") {

    // A --> B --> D
    //       ^   ^
    //      /   /
    // C --/---/

    JCallGraphRecorder sut;
    sut.SetEnabled();
    //sut.AddToCallGraph({"BName","BTag","AName","ATag"});
    //sut.AddToCallGraph({"BName","BTag","CName","CTag"});
    //sut.AddToCallGraph({"DName","DTag","BName","BTag"});
    //sut.AddToCallGraph({"DName","DTag","CName","CTag"});
    JCallGraphRecorder::JCallGraphNode("BName", "BTag", "AName", "ATag");
    JCallGraphRecorder::JCallGraphNode("BName", "BTag", "CName", "CTag");
    JCallGraphRecorder::JCallGraphNode("DName", "DTag", "BName", "BTag");
    JCallGraphRecorder::JCallGraphNode("DName", "DTag", "CName", "CTag");
    REQUIRE(sut.GetCallGraph().size() == 4);

    auto result = sut.TopologicalSort();
    REQUIRE(result.size() == 4);
    REQUIRE(result[0].first == "AName");
    REQUIRE(result[1].second == "CTag");
    REQUIRE(result[2].first == "BName");
    REQUIRE(result[3].first == "DName");
}


struct ObjA {};
struct ObjB {};
struct ObjC {};
struct ObjD {};

struct FacA: public JFactoryT<ObjA> {
    void Process(const std::shared_ptr<const JEvent>& event) override {
    }
};

struct FacB: public JFactoryT<ObjB> {
    FacB() {
        SetTag("WeirdBTag");
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        event->Get<ObjA>();
    }
};

struct FacC: public JFactoryT<ObjC> {
    void Process(const std::shared_ptr<const JEvent>& event) override {
    }
};

struct FacD: public JFactoryT<ObjD> {
    void Process(const std::shared_ptr<const JEvent>& event) override {
        event->Get<ObjB>("WeirdBTag");
        event->Get<ObjC>();
    }
};


TEST_CASE("Test topological sort algorithm using actual Factories") {
    JApplication app;
    JFactorySet factories;
    factories.Add(new FacA());
    factories.Add(new FacB());
    factories.Add(new FacC());
    factories.Add(new FacD());
    auto event = std::make_shared<JEvent>(&app);
    event->SetFactorySet(&factories);
    event->GetJCallGraphRecorder()->SetEnabled();
    event->Get<ObjD>();
    auto result = event->GetJCallGraphRecorder()->TopologicalSort();
    REQUIRE(result.size() == 4);
    REQUIRE(result[0].first == "ObjA");
    REQUIRE(result[1].first == "ObjC");
    REQUIRE(result[2].first == "ObjB");
    REQUIRE(result[2].second == "WeirdBTag");
    REQUIRE(result[3].first == "ObjD");
}

