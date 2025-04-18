// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JApplication.h>
#include "JANA/JException.h"
#include "JANA/JFactoryGenerator.h"
#include "catch.hpp"

namespace jana::components::getobjects_tests {

struct Obj : public JObject { int data; };

struct Src : public JEventSource {

    bool emit_inserts_rawdata = true;
    bool getobjects_retrieves_rawdata = true;

    Src() {
        EnableGetObjects();
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent& event) override {
        if (emit_inserts_rawdata) {
            auto item_to_insert = new Obj;
            item_to_insert->data = 33 + event.GetEventNumber();
            event.Insert(item_to_insert, "raw");
        }
        return Result::Success;
    }
    bool GetObjects(const std::shared_ptr<const JEvent>& event, JFactory* fac) override {

        LOG_INFO(GetLogger()) << "GetObjects: Fac has object name '" << fac->GetObjectName() << "' and type name '" << fac->GetTypeName() << "' and tag '" << fac->GetTag() << "'" << LOG_END;

        if (getobjects_retrieves_rawdata) {
            event->Get<Obj>("raw");
        }

        //if (fac->GetObjectName() == "jana::components::getobjects_tests::Obj") {2
        auto typed_fac = dynamic_cast<JFactoryT<Obj>*>(fac);
        if (typed_fac != nullptr) {
            auto obj = new Obj;
            obj->data = 22;
            typed_fac->Insert(obj);
            return true;
        }
        return false;
    }
};

struct Fac : public JFactoryT<Obj> {
    void Process(const std::shared_ptr<const JEvent>&) override {
        auto obj = new Obj;
        obj->data = 23;
        Insert(obj);
    }
};

struct RFac : public JFactoryT<Obj> {
    RFac() {
        SetFactoryFlag(REGENERATE);
    }
    void Process(const std::shared_ptr<const JEvent>&) override {
        auto obj = new Obj;
        obj->data = 23;
        Insert(obj);
    }
};

struct Proc : public JEventProcessor {
    bool from_getobjects=true;
    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto objs = event->Get<Obj>();
        REQUIRE(objs.size() == 1);
        if (from_getobjects) {
            REQUIRE(objs[0]->data == 22);
        }
        else {
            REQUIRE(objs[0]->data == 23);
        }
    }
};

TEST_CASE("GetObjectsTests_NoFac") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "warn");
    app.SetParameterValue("jana:nevents", 1);
    app.Add(new Src);
    auto proc = new Proc;
    proc->from_getobjects = true;
    app.Add(proc);
    app.Add(new JFactoryGeneratorT<JFactoryT<Obj>>);
    app.Run();
}

TEST_CASE("GetObjectsTests_OverrideFac") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "warn");
    app.SetParameterValue("jana:nevents", 1);
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    auto proc = new Proc;
    proc->from_getobjects = true;
    app.Add(proc);
    app.Run();
}

TEST_CASE("GetObjectsTests_Regenerate") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "warn");
    app.SetParameterValue("jana:nevents", 1);
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<RFac>);
    auto proc = new Proc;
    proc->from_getobjects = false;
    app.Add(proc);
    app.Run();
}

TEST_CASE("GetObjectsTests_GetObjectsNeedsMissingData") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "warn");
    app.SetParameterValue("jana:nevents", 1);

    auto src = new Src;
    src->emit_inserts_rawdata = false;
    src->getobjects_retrieves_rawdata = true;

    auto proc = new Proc;
    proc->from_getobjects = true;

    app.Add(src);
    app.Add(proc);
    app.Add(new JFactoryGeneratorT<JFactoryT<Obj>>);
    app.Add(new JFactoryGeneratorT<JFactoryT<Obj>>("raw"));
    try {
        app.Run();
        REQUIRE(0 == 1);
    }
    catch (const JException& e){
        LOG << e;
        REQUIRE(e.message == "Encountered a cycle in the factory dependency graph! Hint: Maybe this data was supposed to be inserted in the JEventSource");
        REQUIRE(e.type_name == "JFactoryT<jana::components::getobjects_tests::Obj>");
    }
}


} // namespace ...
