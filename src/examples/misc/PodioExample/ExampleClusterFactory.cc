
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "ExampleClusterFactory.h"
#include "PodioDatamodel/ExampleClusterCollection.h"
#include "PodioDatamodel/ExampleHitCollection.h"
#include <JANA/JEvent.h>

ExampleClusterFactory::ExampleClusterFactory() {
    SetTag("clusters");
    // TODO: Need to throw an exception if you try to register a PODIO factory with an empty or non-unique tag
}

void ExampleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {

    // This example groups hits according to the quadrant in which they lie on the x-y plane.

    MutableExampleCluster quadrant1, quadrant2, quadrant3, quadrant4;

    auto hits = event->GetCollection<ExampleHit>("hits");
    for (auto hit : *hits) {
        if (hit.x() > 0) {
            if (hit.y() > 0) {
                quadrant1.addHits(hit);
                quadrant1.energy(quadrant1.energy() + hit.energy());
            }
            else {
                quadrant4.addHits(hit);
                quadrant4.energy(quadrant4.energy() + hit.energy());
            }
        }
        else {
            if (hit.y() > 0) {
                quadrant2.addHits(hit);
                quadrant2.energy(quadrant2.energy() + hit.energy());

            }
            else {
                quadrant3.addHits(hit);
                quadrant3.energy(quadrant3.energy() + hit.energy());
            }

        }
    }
    ExampleClusterCollection clusters;
    if (quadrant1.Hits_size() > 0) clusters.push_back(quadrant1);
    if (quadrant2.Hits_size() > 0) clusters.push_back(quadrant2);
    if (quadrant3.Hits_size() > 0) clusters.push_back(quadrant3);
    if (quadrant4.Hits_size() > 0) clusters.push_back(quadrant4);

    // If no hits were assigned to a cluster, it will self-destruct when it goes out of scope
    SetCollection(std::move(clusters));
}


