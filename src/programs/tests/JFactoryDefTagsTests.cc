
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JFactoryT.h>
#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>
#include "JANA/JFactoryGenerator.h"

struct Obj : public JObject {
    JOBJECT_PUBLIC(Obj)
    double x=0,y=0,E=0;
};

struct Fac1 : public JFactoryT<Obj> {
    void Process(const std::shared_ptr<const JEvent>&) override {
        auto obj = new Obj;
        obj->x = 1;
        obj->y = 1;
        obj->E = 22.2;
        Insert(obj);
    }
};

struct Fac2 : public JFactoryT<Obj> {
    Fac2() {
        SetTag("tagB");
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto obj = new Obj;
        obj->x = 1;
        obj->y = 1;
        obj->E = 33.3;
        Insert(obj);
    }
};

TEST_CASE("SmallDefTags") {

    auto event = std::make_shared<JEvent>();
    auto fs = new JFactorySet;
    fs->Add(new Fac1);
    fs->Add(new Fac2);
    event->SetFactorySet(fs);

    auto objsA = event->Get<Obj>("");
    REQUIRE(objsA[0]->E == 22.2);

    auto objsB = event->Get<Obj>("tagB");
    REQUIRE(objsB[0]->E == 33.3);

    std::map<std::string, std::string> deftags;
    deftags["Obj"] = "tagB";
    event->SetDefaultTags(deftags);
    auto objsC = event->Get<Obj>("");
    REQUIRE(objsC[0]->E == 33.3);

}

TEST_CASE("MediumDefTags") {
    JApplication app;
    app.Add(new JFactoryGeneratorT<Fac1>);
    app.Add(new JFactoryGeneratorT<Fac2>);
    app.SetParameterValue("DEFTAG:Obj", "tagB");
    app.Initialize();
    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);
    auto objs = event->Get<Obj>();
    REQUIRE(objs[0]->E == 33.3);
}


struct DummySource : public JEventSource {
    DummySource() : JEventSource("DummySource") {};
    void GetEvent(std::shared_ptr<JEvent>) override {};
};

struct DummyProcessor : public JEventProcessor {
    double E;
    std::atomic_int processed_count {0};
    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto objsA = event->Get<Obj>("");
        E = objsA[0]->E;
        processed_count++;
    }
};


TEST_CASE("LargeDefTags") {

    JApplication app;
    app.Add(new JFactoryGeneratorT<Fac1>);
    app.Add(new JFactoryGeneratorT<Fac2>);
    app.Add(new DummySource);
    auto proc = new DummyProcessor;
    app.Add(proc);
    app.SetParameterValue("jana:nevents", 1);
    app.SetParameterValue("deftag:Obj", "tagB");
    app.Run(true);
    REQUIRE(proc->processed_count == 1);
    REQUIRE(proc->E == 33.3);
    app.Quit();
}