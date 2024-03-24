// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/Omni/JOmniFactory.h>


struct MyEventFactory : public JOmniFactory<MyEventFactory> {

    PodioInput<ExampleCluster> protoclusters_in {this, "evt_protoclusters", JEventLevel::Event};
    PodioOutput<ExampleCluster> clusters_out {this, "clusters"};


    MyEventFactory() {
        SetLevel(JEventLevel::Event);
    }

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {


        auto cs = std::make_unique<ExampleClusterCollection>();
        if (protoclusters_in()->size() != 1) throw JException("Wrong size!");
        auto pc = protoclusters_in()->at(0);
        auto c = MutableExampleCluster(pc.energy() + 1000);
        c.addClusters(pc);
        cs->push_back(c);

        clusters_out() = std::move(cs);
    }
};


