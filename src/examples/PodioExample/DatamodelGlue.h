
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
struct PodioCollectionMap {
};

template <>
struct PodioCollectionMap<ExampleHitCollection> {
    using contents_t = ExampleHit;
};
template <>
struct PodioCollectionMap<ExampleClusterCollection> {
    using contents_t = ExampleCluster;
};
template <>
struct PodioCollectionMap<EventInfoCollection> {
    using contents_t = EventInfo;
};

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
    using mutable_t = MutableEventInfo;
    using collection_t = EventInfoCollection;
};


template<typename ... Ts>
struct Overload : Ts ... {
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

template <typename F, typename... ArgsT>
void visitPodioType(const std::string& podio_typename, F& helper, ArgsT... args) {
    if (podio_typename == "EventInfo") {
        return helper.template operator()<EventInfo>(std::forward<ArgsT>(args)...);
    }
    else if (podio_typename == "ExampleHit") {
        return helper.template operator()<ExampleHit>(std::forward<ArgsT>(args)...);
    }
    else if (podio_typename == "ExampleCluster") {
        return helper.template operator()<ExampleCluster>(std::forward<ArgsT>(args)...);
    }
    throw std::runtime_error("Not a podio typename!");
}

template <typename Visitor>
struct DatamodelCollectionVisit {
    void operator()(const podio::CollectionBase &collection, Visitor& visitor) {
        std::string podio_typename = collection.getTypeName();
        if (podio_typename == "EventInfoCollection") {
            return visitor(static_cast<const EventInfoCollection &>(collection));
        } else if (podio_typename == "ExampleHitCollection") {
            return visitor(static_cast<const ExampleHitCollection &>(collection));
        } else if (podio_typename == "ExampleClusterCollection") {
            return visitor(static_cast<const ExampleClusterCollection &>(collection));
        }
        throw std::runtime_error("Unrecognized podio typename!");
    }
};

#endif //JANA2_DATAMODELGLUE_H
