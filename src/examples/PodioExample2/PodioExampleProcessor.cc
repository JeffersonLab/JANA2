
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "PodioExampleProcessor.h"

/*
struct PrintingVisitor {

    template <typename T>
    void operator() (const podio::CollectionBase* collection, std::string collection_name) {

        auto* typed_collection = static_cast<const typename PodioTypeMap<T>::collection_t*>(collection);

        std::cout << collection_name << " :: " << collection->getValueTypeName() << " = [";
        for (const T& object : *typed_collection) {
            std::cout << object.id() << ", ";
        }
        std::cout << "]" << std::endl;
    }

    template <>
    void operator()<ExampleCluster>(const podio::CollectionBase* collection, std::string collection_name) {

        std::cout << collection_name << " :: " << collection->getValueTypeName() << std::endl;
        auto* typed_collection = static_cast<const ExampleClusterCollection*>(collection);
        for (const ExampleCluster& cluster : *typed_collection) {
            std::cout << "    " << cluster.id() << ": energy=" << cluster.energy() << ", hits=" << std::endl;
            for (const ExampleHit& hit : cluster.Hits()) {
                std::cout << "        " << hit.id() << ": energy=" << hit.energy() << std::endl;
            }
        }
    }

};
*/


// TODO: C++20 visitor using a lambda overload set
// TODO: Less generic visitor, e.g. VisitCollection

void PodioExampleProcessor::Process(const std::shared_ptr<const JEvent> &event) {

    // Obtain a typed collection just like you would in a JFactory
    auto hits = event->GetCollection<ExampleHit>("hits");
    auto hits_filtered = event->GetCollection<ExampleHit>("hits_filtered");
    auto clusters = event->GetCollection<ExampleCluster>("clusters");
    auto clusters_filtered = event->GetCollection<ExampleCluster>("clusters_filtered");
    auto clusters_from_hits_filtered = event->GetCollection<ExampleCluster>("clusters_from_hits_filtered");

    std::cout << "========================" << std::endl;
    std::cout << "Event nr: " << event->GetEventNumber() << ", Cluster count: " << clusters->size() << std::endl;

    std::cout << "hits:" << std::endl;
    for (const ExampleHit& hit : *hits) {
        std::cout << "    " << hit.id() << ": energy=" << hit.energy() << ", x=" << hit.x() << ", y=" << hit.y() << std::endl;
    }
    std::cout << "hits_filtered:" << std::endl;
    for (const ExampleHit& hit : *hits_filtered) {
        std::cout << "    " << hit.id() << ": energy=" << hit.energy() << ", x=" << hit.x() << ", y=" << hit.y() << std::endl;
    }

    std::cout << "clusters:" << std::endl;
    for (const ExampleCluster& cluster : *clusters) {
        std::cout << "    " << cluster.id() << ": energy=" << cluster.energy() << ", hits=" << std::endl;
        for (const ExampleHit& hit : cluster.Hits()) {
            std::cout << "        " << hit.id() << ": energy=" << hit.energy() << ", x=" << hit.x() << ", y=" << hit.y() << std::endl;
        }
    }
    std::cout << "clusters_filtered:" << std::endl;
    for (const ExampleCluster& cluster : *clusters_filtered) {
        std::cout << "    " << cluster.id() << ": energy=" << cluster.energy() << ", hits=" << std::endl;
        for (const ExampleHit& hit : cluster.Hits()) {
            std::cout << "        " << hit.id() << ": energy=" << hit.energy() << ", x=" << hit.x() << ", y=" << hit.y() << std::endl;
        }
    }
    std::cout << "clusters_from_hits_filtered:" << std::endl;
    for (const ExampleCluster& cluster : *clusters_from_hits_filtered) {
        std::cout << "    " << cluster.id() << ": energy=" << cluster.energy() << ", hits=" << std::endl;
        for (const ExampleHit& hit : cluster.Hits()) {
            std::cout << "        " << hit.id() << ": energy=" << hit.energy() << ", x=" << hit.x() << ", y=" << hit.y() << std::endl;
        }
    }
}
