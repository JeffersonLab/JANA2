//
// Created by Nathan Brei on 2019-11-21.
//

#ifndef JANA2_JFACTORYTESTS_H
#define JANA2_JFACTORYTESTS_H

#include <JANA/JObject.h>
#include <JANA/JFactoryT.h>

/// DummyObject is a trivial JObject which reports its own destruction.
struct DummyObject : public JObject {

    int data;
    bool* is_destroyed_flag = nullptr;

    DummyObject(int data, bool* is_destroyed_flag=nullptr) : data(data), is_destroyed_flag(is_destroyed_flag) {}
    ~DummyObject() {
        if (is_destroyed_flag != nullptr) {
            *is_destroyed_flag = true;
        }
    }
};



/// DummyFactory is a trivial JFactory which reports how often its member functions get called
/// and whether any of the JObjects were destroyed
struct DummyFactory : public JFactoryT<DummyObject> {

    int init_call_count = 0;
    int change_run_call_count = 0;
    int process_call_count = 0;
    bool destroy_flags[3] = {false, false, false};

    void Init() override {
        ++init_call_count;
    }

    void ChangeRun(const std::shared_ptr<const JEvent>& event) override {
        ++change_run_call_count;
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        ++process_call_count;
        Insert(new DummyObject (7, &destroy_flags[0]));
        Insert(new DummyObject (22, &destroy_flags[1]));
        Insert(new DummyObject (23, &destroy_flags[2]));
    }
};


struct DummySource: public JEventSource {

    DummySource() : JEventSource("dummy", nullptr) {}

    void GetEvent(std::shared_ptr<JEvent>) override {
    };

    bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory) override {
        auto factory = dynamic_cast<JFactoryT<DummyObject>*>(aFactory);
        if (factory != nullptr) {
            factory->Insert(new DummyObject(8));
            factory->Insert(new DummyObject(88));
        }
        return false;
    }
};


#endif //JANA2_JFACTORYTESTS_H
