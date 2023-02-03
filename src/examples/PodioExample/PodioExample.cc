
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
        m_frame.put(std::move(*coll), coll_name);
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

struct PrintingVisitor {
    template <typename T>
    void operator() (const podio::CollectionBase* coll, std::string coll_name) {
        using CollT = const typename PodioTypeMap<T>::collection_t;
        CollT* typed_col = static_cast<CollT*>(coll);

        std::cout << coll_name << std::endl;
        for (const T& object : *typed_col) {
            std::cout << coll->getValueTypeName() << std::endl;
            std::cout << object << std::endl;
        }
    }
};


int main() {
    FakeJANA jannah;
    jannah.Process();

    PrintingVisitor printer;

    std::cout << "WRITING TO ROOT FILE:" << std::endl;
    for (const std::string& coll_name : jannah.m_frame.getAvailableCollections()) {
        const podio::CollectionBase* coll = jannah.m_frame.get(coll_name);
        visitPodioType(coll->getValueTypeName(), printer, coll, coll_name);
    }

    podio::ROOTFrameWriter writer("podio_output.root");
    writer.writeFrame(jannah.m_frame, "events");
    writer.finish();

    podio::ROOTFrameReader reader;
    reader.openFile("podio_output.root");
    auto framedata = reader.readEntry("events", 0);
    auto frame2 = podio::Frame(std::move(framedata));

    std::cout << "READ FROM ROOT FILE:" << std::endl;
    for (const std::string& coll_name : frame2.getAvailableCollections()) {
        const podio::CollectionBase* coll = frame2.get(coll_name);
        visitPodioType(coll->getValueTypeName(),
               [&]<typename T>(const podio::CollectionBase* coll, std::string name) {
                   using CollT = const typename PodioTypeMap<T>::collection_t;
                   CollT* typed_col = static_cast<CollT*>(coll);

                   std::cout << name << std::endl;
                   for (const T& object : *typed_col) {
                       std::cout << coll->getValueTypeName() << std::endl;
                       std::cout << object << std::endl;
                   }
            }, coll, coll_name);
    }

    // auto untyped_coll = frame2.get("hits");
    // std::cout << "ExampleHit collection: Typename=" << untyped_coll->getTypeName() << ", DataTypeName=" << untyped_coll->getDataTypeName() << ", ValueTypeName=" << untyped_coll->getValueTypeName() << std::endl;
}

