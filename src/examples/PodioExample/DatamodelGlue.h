
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_DATAMODELGLUE_H
#define JANA2_DATAMODELGLUE_H

#include <PodioDatamodel/ExampleHitCollection.h>
#include <PodioDatamodel/ExampleClusterCollection.h>
#include <PodioDatamodel/EventInfoCollection.h>
#include <PodioDatamodel/TimesliceInfoCollection.h>


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
    if (podio_typename == "TimesliceInfo") {
        return helper.template operator()<TimesliceInfo>(std::forward<ArgsT>(args)...);
    }
    else if (podio_typename == "ExampleHit") {
        return helper.template operator()<ExampleHit>(std::forward<ArgsT>(args)...);
    }
    else if (podio_typename == "ExampleCluster") {
        return helper.template operator()<ExampleCluster>(std::forward<ArgsT>(args)...);
    }
    throw std::runtime_error("Not a podio typename!");
}

// If you are using C++20, you can use templated lambdas to write your visitor completely inline like so:
/*
           visitPodioType(coll->getValueTypeName(),
               [&]<typename T>(const podio::CollectionBase* coll, std::string name) {
                   using CollT = const typename T::collection_type;
                   CollT* typed_col = static_cast<CollT*>(coll);

                   std::cout << name << std::endl;
                   for (const T& object : *typed_col) {
                       std::cout << coll->getValueTypeName() << std::endl;
                       std::cout << object << std::endl;
                   }
            }, coll, coll_name);
 */

template <typename Visitor>
struct VisitExampleDatamodel {
    void operator()(Visitor& visitor, const podio::CollectionBase &collection) {
        const auto podio_typename = collection.getTypeName();
        if (podio_typename == "EventInfoCollection") {
            return visitor(static_cast<const EventInfoCollection &>(collection));
        } else if (podio_typename == "TimesliceInfoCollection") {
            return visitor(static_cast<const TimesliceInfoCollection &>(collection));
        } else if (podio_typename == "ExampleHitCollection") {
            return visitor(static_cast<const ExampleHitCollection &>(collection));
        } else if (podio_typename == "ExampleClusterCollection") {
            return visitor(static_cast<const ExampleClusterCollection &>(collection));
        }
        throw std::runtime_error("Unrecognized podio typename!");
    }
};


#endif //JANA2_DATAMODELGLUE_H
