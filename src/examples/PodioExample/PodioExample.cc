
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
        auto coll = new ExampleHitCollection;
        MutableExampleHit hit;
        hit.cellID(22);
        hit.energy(100.0);
        hit.x(0);
        hit.y(0);
        hit.z(0);
        std::cout << hit << std::endl;
        coll->push_back(hit);
        SetCollection<ExampleHit>(coll, "hits");

    }

    template <typename T>
    void SetCollection(typename PodioTypeMap<T>::collection_t* coll, std::string coll_name) {
        const auto& moved = m_frame.put(std::move(*coll), coll_name);
        for (const ExampleHit& item : moved) {
            m_cache.push_back(&item); // Is this stable?
        }
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
    std::cout << "Hello world" << std::endl;
    auto coll = new ExampleHitCollection;
    MutableExampleHit hit;
    hit.cellID(22);
    hit.energy(100.0);
    hit.x(0);
    hit.y(0);
    hit.z(0);
    std::cout << hit << std::endl;
    coll->push_back(hit);
    podio::Frame frame;
    frame.put(std::move(*coll), "SillyCollection");

    podio::ROOTFrameWriter writer("podio_output.root");
    writer.writeFrame(frame, "events");
    writer.finish();

    podio::ROOTFrameReader reader;
    reader.openFile("podio_output.root");
    auto framedata = reader.readEntry("events", 0);
    auto frame2 = podio::Frame(std::move(framedata));

    auto& readcol = frame2.get<ExampleHitCollection>("SillyCollection");
    const auto& readhit = readcol[0];
    std::cout << readhit << std::endl;

}

