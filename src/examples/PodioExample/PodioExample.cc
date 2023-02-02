
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#include <iostream>
#include <podio/CollectionBase.h>
#include <podio/Frame.h>
#include "datamodel/MutableExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include <podio/ROOTFrameWriter.h>
#include <podio/ROOTFrameReader.h>
#include "DatamodelGlue.h"

struct FakeJANA {
    podio::Frame m_frame;
    std::vector<const ExampleHit*> m_cache;

    void Process() {
        MutableExampleHit hit;
        hit.cellID(22);
        hit.energy(100.0);
        hit.x(0);
        hit.y(0);
        hit.z(0);

        MutableExampleHit hit2;
        hit2.cellID(49);
        hit2.energy(97.1);
        hit2.x(1);
        hit2.y(2);
        hit2.z(3);

        auto coll = new ExampleHitCollection;
        coll->push_back(hit);
        coll->push_back(hit2);

        auto cluster_coll = new ExampleClusterCollection;
        MutableExampleCluster cluster;
        cluster.addHits(hit);
        cluster.addHits(hit2);
        cluster_coll->push_back(cluster);

        SetCollection<ExampleHit>(coll, "hits");
        SetCollection<ExampleCluster>(cluster_coll, "clusters");
    }

    template <typename T>
    void SetCollection(typename PodioTypeMap<T>::collection_t* coll, std::string coll_name) {
        const auto& moved = m_frame.put(std::move(*coll), coll_name);
        // for (const ExampleHit& item : moved) {
        //    m_cache.push_back(&item); // Is this stable?
        //}
    }

    template <typename T>
    void Set(const std::vector<T*>& hits) {
        auto coll = new typename PodioTypeMap<T>::collection_t;
        for (T* hit : hits) {
            coll->push_back(hit);
        }
    }

    template <typename T>
    const typename PodioTypeMap<T>::collection_t& GetCollection() {
        return m_frame.get<PodioTypeMap<T>::collection_t>("FakeFactory");
    }


};

int main() {
    FakeJANA jannah;
    jannah.Process();

    std::cout << "WRITING TO ROOT FILE:" << std::endl;
    for (const auto& hit : jannah.m_frame.get<ExampleHitCollection>("hits")) {
        std::cout << "HIT:" << std::endl << hit << std::endl;
    }
    for (const auto& cluster : jannah.m_frame.get<ExampleClusterCollection>("clusters")) {
        std::cout << "CLUSTER:" << std::endl << cluster << std::endl;
    }

    podio::ROOTFrameWriter writer("podio_output.root");
    writer.writeFrame(jannah.m_frame, "events");
    writer.finish();

    podio::ROOTFrameReader reader;
    reader.openFile("podio_output.root");
    auto framedata = reader.readEntry("events", 0);
    auto frame2 = podio::Frame(std::move(framedata));

    std::cout << "READ FROM ROOT FILE:" << std::endl;
    for (const auto& hit : frame2.get<ExampleHitCollection>("hits")) {
        std::cout << "HIT:" << std::endl << hit << std::endl;
    }
    for (const auto& cluster : frame2.get<ExampleClusterCollection>("clusters")) {
        std::cout << "CLUSTER:" << std::endl << cluster << std::endl;
    }
}

