
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

template <typename F, typename... ArgsT>
void visitPodioType(const std::string& podio_typename, F&& helper, ArgsT... args) {
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

#endif //JANA2_DATAMODELGLUE_H
