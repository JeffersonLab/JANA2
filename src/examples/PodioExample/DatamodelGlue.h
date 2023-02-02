
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_DATAMODELGLUE_H
#define JANA2_DATAMODELGLUE_H

#include <datamodel/ExampleHit.h>
#include <datamodel/ExampleHitCollection.h>
#include <datamodel/ExampleCluster.h>
#include <datamodel/ExampleClusterCollection.h>
#include <datamodel/EventInfo.h>
#include <datamodel/EventInfoCollection.h>

template <typename T>
struct PodioTypeMap {
};

template <>
struct PodioTypeMap<ExampleHit> {
    using mutable_t = MutableExampleHit;
    using collection_t = ExampleHitCollection;
};

template <>
struct PodioTypeMap<ExampleCluster> {
    using mutable_t = MutableExampleCluster;
    using collection_t = ExampleClusterCollection;
};

template <>
struct PodioTypeMap<EventInfo> {
    using mutable_t = EventInfoCollection;
    using collection_t = MutableEventInfo;
};



#endif //JANA2_DATAMODELGLUE_H
