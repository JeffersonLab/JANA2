// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JEventSource.h>
#include "catch.hpp"

struct Obj1 : public JObject { int data; };
struct Obj2 : public JObject { int data; };
struct Obj3 : public JObject { int data; };
struct Obj4 : public JObject { int data; };

class Src : public JEventSource {
    Src() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent& event) override {
        auto obj = new Obj1;
        obj->data = 21;
        event.Insert(obj);
        return Result::Success;
    }
    bool GetObjects(const std::shared_ptr<const JEvent>&, JFactory* fac) override {
        if (fac->GetObjectName() == "Obj2") {
            auto obj = new Obj2;
            obj->data = 22;
            fac->Insert(obj);
            return true;
        }
        return false;
    }
};

class Fac : public JFactoryT<Obj3> {
    void Process(const std::shared_ptr<const JEvent>&) override {
        auto obj = new Obj3;
        obj->data = 23;
        Insert(obj);
    }
};

TEST_CASE("GetObjectsTests") {
    JEvent event;
    JFactorySet* fs = new JFactorySet;
    fs->Add(new Fac);
    event.SetFactorySet(fs);
    event.GetJCallGraphRecorder()->SetEnabled(true);

}

