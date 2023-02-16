
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "ExampleClusterFactory.h"
#include "datamodel/ExampleHit.h"
#include <JANA/JEvent.h>

ExampleClusterFactory::ExampleClusterFactory() {
    SetTag("clusters");
    // TODO: Need to throw an exception if you try to register a PODIO factory with an empty or non-unique tag
}

void ExampleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {
    auto hits = event->GetCollection<ExampleHit>("hits");
    MutableExampleCluster cluster;
    for (auto hit : *hits) {
        cluster.addHits(hit);
    }
    auto* clusters = new ExampleClusterCollection();
    clusters->push_back(cluster);
    SetCollection(clusters);
}


// TODO: Expose collections as refs, not ptrs?