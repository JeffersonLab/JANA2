// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <PodioDatamodel/ExampleHitCollection.h>
#include <PodioDatamodel/ExampleClusterCollection.h>


struct PodioProtoclusteringFactory : public JOmniFactory<PodioProtoclusteringFactory> {

    PodioInput<ExampleHit> hits_in {this};
    PodioOutput<ExampleCluster> clusters_out {this};

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        auto cs = std::make_unique<ExampleClusterCollection>();
        for (auto hit : *hits_in()) {
            auto cluster = cs->create();
            cluster.energy(hit.energy());
            cluster.addHits(hit);
        }
        clusters_out() = std::move(cs);
    }
};


