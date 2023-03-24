
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "ExampleMultifactory.h"
#include <JANA/JEvent.h>
#include "DatamodelGlue.h"

ExampleMultifactory::ExampleMultifactory() {

    // We need to declare upfront what our Multifactory produces.
    DeclarePodioOutput<ExampleHit>("hits_filtered", false);
    DeclarePodioOutput<ExampleCluster>("clusters_from_hits_filtered");
    DeclarePodioOutput<ExampleCluster>("clusters_filtered", false);
}

void ExampleMultifactory::Process(const std::shared_ptr<const JEvent> & event) {

    auto hits = event->GetCollection<ExampleHit>("hits");

    // We are going to write back a collection of hits that are simply a filtered version of the other ones
    // These are already owned by the 'hits' collection, and we are smart enough to not have to duplicate them.
    // We have to set the SubsetCollection property on our new collection before we can add the owned PODIO objects to it.
    auto hits_filtered = new ExampleHitCollection;
    hits_filtered->setSubsetCollection(true);
    for (auto hit : *hits) {
        if (hit.energy() > 10) {
            hits_filtered->push_back(hit);
        }
    }

    auto clusters_from_hits_filtered = new ExampleClusterCollection;
    MutableExampleCluster quadrant1, quadrant2, quadrant3, quadrant4;

    for (const auto& hit : *hits_filtered) {
        if (hit.x() > 0 && hit.y() > 0) {
            quadrant1.addHits(hit);
            quadrant1.energy(quadrant1.energy() + hit.energy());
        }
        else if (hit.x() > 0 && hit.y() <= 0) {
            quadrant4.addHits(hit);
            quadrant4.energy(quadrant4.energy() + hit.energy());
        }
        else if (hit.x() <= 0 && hit.y() > 0) {
            quadrant2.addHits(hit);
            quadrant2.energy(quadrant2.energy() + hit.energy());
        }
        else if (hit.x() <= 0 && hit.y() <= 0) {
            quadrant3.addHits(hit);
            quadrant3.energy(quadrant3.energy() + hit.energy());
        }
    }

    if (quadrant1.Hits_size() > 0) clusters_from_hits_filtered->push_back(quadrant1);
    if (quadrant2.Hits_size() > 0) clusters_from_hits_filtered->push_back(quadrant2);
    if (quadrant3.Hits_size() > 0) clusters_from_hits_filtered->push_back(quadrant3);
    if (quadrant4.Hits_size() > 0) clusters_from_hits_filtered->push_back(quadrant4);


    auto clusters = event->GetCollection<ExampleCluster>("clusters");
    auto clusters_filtered = new ExampleClusterCollection;
    clusters_filtered->setSubsetCollection(true);

    for (auto cluster : *clusters) {
        if (cluster.energy() > 50) {
            clusters_filtered->push_back(cluster);
        }
    }

    // We have to do this last because when we call SetCollection, we transfer ownership of the collection pointers to
    // the PODIO frame, which immediately invalidates them
    SetCollection<ExampleHit>("hits_filtered", hits_filtered);
    SetCollection<ExampleCluster>("clusters_from_hits_filtered", clusters_from_hits_filtered);
    SetCollection<ExampleCluster>("clusters_filtered", clusters_filtered);

}
