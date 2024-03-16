// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "MyDataModel.h"
#include <JANA/JEventSource.h>


struct MyTimesliceSource : public JEventSource {

    MyTimesliceSource(std::string source_name, JApplication *app) : JEventSource(source_name, app) { 
        SetLevel(JEventLevel::Timeslice);
    }

    static std::string GetDescription() { return "MyTimesliceSource"; }

    std::string GetType(void) const override { return JTypeInfo::demangle<decltype(*this)>(); }

    void Open() override { }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto evt = event->GetEventNumber();
        std::vector<MyInput*> inputs;
        inputs.push_back(new MyInput(22,3.6,evt,0));
        inputs.push_back(new MyInput(23,3.5,evt,1));
        inputs.push_back(new MyInput(24,3.4,evt,2));
        inputs.push_back(new MyInput(25,3.3,evt,3));
        inputs.push_back(new MyInput(26,3.2,evt,4));
        event->Insert(inputs);

        auto hits = std::make_unique<ExampleHitCollection>();
        hits.push_back(ExampleHit(22));
        hits.push_back(ExampleHit(23));
        hits.push_back(ExampleHit(24));
        event->InsertCollection(hits);

        jout << "MyTimesliceSource: Emitting " << event->GetEventNumber() << jendl;
    }
};
