// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/Omni/JOmniFactory.h>


struct MyTimesliceFactory : public JOmniFactory<MyTimesliceFactory> {

    PodioInput<ExampleHit> hits_in {this, "hits", JEventLevel::Timeslice};
    PodioOutput<ExampleCluster> clusters_out {this, "protoclusters"};

    MyTimesliceFactory() {
        SetLevel(JEventLevel::Timeslice);
    }

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        auto cs = std::make_unique<ExampleClusterCollection>();
        auto hit = hits_in()->at(0);
        auto cluster = MutableExampleCluster(hit.energy());
        cluster.addHits(hit);
        cs->push_back(cluster);
        clusters_out() = std::move(cs);
    }
};


