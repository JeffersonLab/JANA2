


// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "MyDataModel.h"
#include <JANA/JFactoryT.h>
#include <JANA/Omni/JOmniFactory.h>


struct MyClusterFactory : public JFactoryT<MyCluster> {

    int init_call_count = 0;
    int change_run_call_count = 0;
    int process_call_count = 0;

    MyClusterFactory() {
        SetLevel(JEventLevel::Event);
    }

    void Init() override {
        ++init_call_count;
    }

    void ChangeRun(const std::shared_ptr<const JEvent>&) override {
        ++change_run_call_count;
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        ++process_call_count;

        auto protos = event->Get<MyCluster>("protos");
        // TODO: Output something sensible
    }
};


