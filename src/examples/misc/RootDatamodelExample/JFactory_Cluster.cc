// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
//
// This implements a simple clusterizer factory to create Cluster
// objects out of Hit objects. The Hit objects are obtained from
// JANA where in principle they could come from either the event
// source (e.g. file) or from another factory. (For this plugin
// though they come from the event source.)
//
// For this example we only implement the Process method for simplicity.

#include <JANA/JEvent.h>
#include "JFactory_Cluster.h"

void JFactory_Cluster::Process(const std::shared_ptr<const JEvent> &event) {

    // Get all Hit objects
    auto hits = event->Get<Hit>();

    // Make clusters from hits.
    // (for this example we just make a single cluster from all hits)
    auto cluster = new Cluster();
    for( auto hit : hits ) cluster->AddHit( hit );

    /// Publish outputs
    std::vector<Cluster*> results;
    results.push_back(cluster); // in real situation we may have more than one cluster
    Set(results);
}
