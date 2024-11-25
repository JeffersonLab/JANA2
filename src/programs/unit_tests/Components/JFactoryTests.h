
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JFACTORYTESTS_H
#define JANA2_JFACTORYTESTS_H

#include <JANA/JObject.h>
#include <JANA/JFactoryT.h>
#include <JANA/JEventSource.h>

/// DummyObject is a trivial JObject which reports its own destruction.
struct JFactoryTestDummyObject : public JObject {

    int data;
    bool* is_destroyed_flag = nullptr;

    JFactoryTestDummyObject(int data, bool* is_destroyed_flag=nullptr) : data(data), is_destroyed_flag(is_destroyed_flag) {}
    ~JFactoryTestDummyObject() {
        if (is_destroyed_flag != nullptr) {
            *is_destroyed_flag = true;
        }
    }
};

struct DifferentDummyObject : public JObject {
    double E;
    DifferentDummyObject(double E) : E(E) {}
};



/// DummyFactory is a trivial JFactory which reports how often its member functions get called
/// and whether any of the JObjects were destroyed
struct JFactoryTestDummyFactory : public JFactoryT<JFactoryTestDummyObject> {

    int init_call_count = 0;
    int change_run_call_count = 0;
    int process_call_count = 0;
    bool destroy_flags[3] = {false, false, false};

    void Init() override {
        ++init_call_count;
    }

    void ChangeRun(const std::shared_ptr<const JEvent>&) override {
        ++change_run_call_count;
    }

    void Process(const std::shared_ptr<const JEvent>&) override {
        ++process_call_count;
        Insert(new JFactoryTestDummyObject (7, &destroy_flags[0]));
        Insert(new JFactoryTestDummyObject (22, &destroy_flags[1]));
        Insert(new JFactoryTestDummyObject (23, &destroy_flags[2]));
    }
};

struct JFactoryTestExceptingFactory : public JFactoryT<JFactoryTestDummyObject> {

    void Process(const std::shared_ptr<const JEvent>&) override {
        throw JException("This was never going to work!");
    }
};

struct JFactoryTestExceptingInInitFactory : public JFactoryT<JFactoryTestDummyObject> {

    void Init() override {
        throw JException("This was never going to initialize even");
    }
    void Process(const std::shared_ptr<const JEvent>&) override {
    }
};


struct JFactoryTestDummySource: public JEventSource {

    int get_objects_count = 0;
    int get_objects_dummy_count = 0;
    int get_objects_different_count = 0;

    JFactoryTestDummySource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        EnableGetObjects();
    }

    Result Emit(JEvent&) override {
        return Result::Success;
    };

    bool GetObjects(const std::shared_ptr<const JEvent>&, JFactory* aFactory) override {
        get_objects_count += 1;
        auto dummy_factory = dynamic_cast<JFactoryT<JFactoryTestDummyObject>*>(aFactory);
        if (dummy_factory != nullptr) {
            get_objects_dummy_count += 1;
            dummy_factory->Insert(new JFactoryTestDummyObject(8));
            dummy_factory->Insert(new JFactoryTestDummyObject(88));
            return true;
        }
        auto different_factory = dynamic_cast<JFactoryT<DifferentDummyObject>*>(aFactory);
        if (different_factory != nullptr) {
            get_objects_different_count += 1;
            different_factory->Insert(new DifferentDummyObject(123.));
            return true;
        }
        return false;
    }
};


#endif //JANA2_JFACTORYTESTS_H
