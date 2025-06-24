// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JOmniFactory.h>
#include <PodioDatamodel/ExampleClusterCollection.h>


struct MyClusterFactory : public JOmniFactory<MyClusterFactory> {

    PodioInput<ExampleCluster> m_protoclusters_in {this};
    PodioOutput<ExampleCluster> m_clusters_out {this};


    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        auto cs = std::make_unique<ExampleClusterCollection>();

        for (auto protocluster : *m_protoclusters_in()) {
            auto cluster = MutableExampleCluster(protocluster.energy() + 1000);
            cluster.addClusters(protocluster);
            cs->push_back(cluster);
        }

        m_clusters_out() = std::move(cs);
    }
};


