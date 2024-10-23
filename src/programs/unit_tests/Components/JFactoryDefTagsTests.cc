
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

namespace deftagstest {
struct Obj : public JObject {
    JOBJECT_PUBLIC(Obj)

    double x = 0, y = 0, E = 0;
};

struct Fac1 : public JFactoryT<Obj> {
    void Process(const std::shared_ptr<const JEvent> &) override {
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

    void Process(const std::shared_ptr<const JEvent>& /*event*/) override {
        auto obj = new Obj;
        obj->x = 1;
        obj->y = 1;
        obj->E = 33.3;
        Insert(obj);
    }
};

struct Fac3 : public JFactoryT<Obj> {
    Fac3() {
        SetTag("tagC");
    }

    void Process(const std::shared_ptr<const JEvent>& /*event*/) override {
        auto obj = new Obj;
        obj->x = 1;
        obj->y = 1;
        obj->E = 4.0;
        Insert(obj);
    }
};
} // namespace deftagstest

TEST_CASE("SmallDefTags") {

    using namespace deftagstest;
    auto event = std::make_shared<JEvent>();
    auto fs = new JFactorySet;
    fs->Add(new Fac1);
    fs->Add(new Fac2);
    fs->Add(new Fac3);
    event->SetFactorySet(fs);

    auto objsA = event->Get<Obj>("");
    REQUIRE(objsA[0]->E == 22.2);

    auto objsB = event->Get<Obj>("tagB");
    REQUIRE(objsB[0]->E == 33.3);

    std::map<std::string, std::string> deftags;
    deftags["deftagstest::Obj"] = "tagB";
    event->SetDefaultTags(deftags);
    auto objsC = event->Get<Obj>();
    REQUIRE(objsC[0]->E == 33.3);

    // Make sure that setting the deftag doesn't interfere with retrieving non-default tags
    auto objsD = event->Get<Obj>("tagC");
    REQUIRE(objsD[0]->E == 4.0);

}

TEST_CASE("MediumDefTags") {
    using namespace deftagstest;
    JApplication app;
    app.Add(new JFactoryGeneratorT<Fac1>);
    app.Add(new JFactoryGeneratorT<Fac2>);
    app.SetParameterValue("DEFTAG:deftagstest::Obj", "tagB");
    auto event = std::make_shared<JEvent>(&app);
    auto objs = event->Get<Obj>();
    REQUIRE(objs[0]->E == 33.3);
}

namespace deftagstest {

struct DummyProcessor : public JEventProcessor {
    double E = 0.5;
    std::atomic_int processed_count{0};
    std::mutex m;

    DummyProcessor() {
        SetTypeName("DummyProcessor");
    }

    void Process(const std::shared_ptr<const JEvent> &event) override {
        auto objsA = event->Get<Obj>("");
        std::lock_guard<std::mutex> lock(m);
        E = objsA[0]->E;
        processed_count++;
    }
};
} // namespace deftagstest


TEST_CASE("LargeDefTags") {

    using namespace deftagstest;
    JApplication app;
    app.Add(new JFactoryGeneratorT<Fac1>);
    app.Add(new JFactoryGeneratorT<Fac2>);
    app.Add(new JEventSource);
    auto proc = new DummyProcessor;
    app.Add(proc);
    app.SetParameterValue("jana:nevents", 3);
    app.SetParameterValue("DEFTAG:deftagstest::Obj", "tagB");
    app.Run(true);
    REQUIRE(proc->processed_count == 3);
    REQUIRE(proc->GetEventCount() == 3);
    REQUIRE(proc->E == 33.3);
}
