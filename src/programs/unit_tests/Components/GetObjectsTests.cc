// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JApplication.h>
#include "JANA/JFactoryGenerator.h"
#include "catch.hpp"

namespace jana::components::getobjects_tests {

struct Obj : public JObject { int data; };

struct Src : public JEventSource {
    Src() {
        EnableGetObjects();
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent&) override {
        return Result::Success;
    }
    bool GetObjects(const std::shared_ptr<const JEvent>&, JFactory* fac) override {

        LOG_INFO(GetLogger()) << "GetObjects: Fac has object name '" << fac->GetObjectName() << "' and type name '" << fac->GetTypeName() << "'" << LOG_END;

        //if (fac->GetObjectName() == "jana::components::getobjects_tests::Obj") {
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

} // namespace ...
