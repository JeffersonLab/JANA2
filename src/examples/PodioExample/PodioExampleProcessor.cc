
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "PodioExampleProcessor.h"


struct PrintingVisitor {
    template <typename T>
    void operator() (const podio::CollectionBase* collection, std::string collection_name) {

        auto* typed_collection = static_cast<const typename PodioTypeMap<T>::collection_t*>(collection);

        std::cout << collection_name << std::endl;
        for (const T& object : *typed_collection) {
            std::cout << collection->getValueTypeName() << std::endl;
            std::cout << object << std::endl;
        }
    }
};

// TODO: C++20 visitor using a lambda overload set
// TODO: Less generic visitor, e.g. VisitCollection

void PodioExampleProcessor::Process(const std::shared_ptr<const JEvent> &event) {

    // Obtain a typed collection just like you would in a JFactory
    auto clusters = event->GetCollection<ExampleCluster>("clusters");
    std::cout << "Event nr: " << event->GetEventNumber() << ", Cluster count: " << clusters->size() << std::endl;

    // Iterate over all collections that have already been constructed (this won't trigger Process())
    // TODO: Add event->GetAllCollections() so that we can trigger the factories

    PrintingVisitor printer;
    // TODO: Visitor should be passed by reference so that we can get output from it
    auto frame = event->GetSingle<podio::Frame>();

    for (const std::string& coll_name : frame->getAvailableCollections()) {
        const podio::CollectionBase* coll = frame->get(coll_name);
        visitPodioType(coll->getValueTypeName(), printer, coll, coll_name);
    }

}
