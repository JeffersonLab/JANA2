
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "ExampleClusterFactory.h"
#include "datamodel/ExampleHit.h"
#include <JANA/JEvent.h>

void ExampleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {
    auto hits = event->GetCollection<ExampleHit>("hits");
    MutableExampleCluster cluster;
    for (auto hit : *hits) {
        cluster.addHits(hit);
    }
    auto* clusters = new ExampleClusterCollection();
    clusters->push_back(cluster);
    SetCollection(event, clusters);
}


// TODO: Expose collections as refs, not ptrs?