
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JSERVICELOCATOR_H_
#define _JSERVICELOCATOR_H_

#include <JANA/JException.h>

#include <map>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <assert.h>
#include <memory>
#include <mutex>


class JServiceLocator;

/// JService is a trait indicating that an object can be shared among JANA components
/// via a simple ServiceLocator. It provides a callback interface for configuring itself
/// when it depends on other JServices.
struct JService {

    /// acquire_services is a callback which allows the user to configure a JService
    /// which relies on other JServices. The idea is that the user uses a constructor
    /// or initialize() method to configure things which don't rely on other JServices, and
    /// then use acquire_services() to configure the things which do. We need this because
    /// due to JANA's plugin architecture, we can't guarantee the order in which JServices
    /// get provided. So instead, we collect all of the JServices first and wire them
    /// together afterwards in a separate phase.
    ///
    /// Note: Don't call JApplication::GetService() or JServiceLocator::get() from InitPlugin()!

    virtual ~JService() = default;
    virtual void acquire_services(JServiceLocator*) {};
};

/// JServiceLocator is a nexus for collecting, initializing, and retrieving JServices.
/// This may be exposed via the JApplication facade, or used on its own. JServiceLocator
/// uses shared-pointer semantics to ensure that JServices always get freed,
/// but also don't get freed prematurely if a JApplication or JServiceLocator go out of scope.
class JServiceLocator {

    std::map<std::type_index, std::pair<std::shared_ptr<JService>, std::once_flag*>> underlying;
    std::mutex mutex;

public:

    ~JServiceLocator() {
        for (auto pair : underlying) {
            delete pair.second.second; // Delete each pointer to once_flag
        }
    }

    template<typename T>
    void provide(std::shared_ptr<T> t) {

        /// Publish a Service to the ServiceLocator. This Service should have
        /// already been constructed, but not finalized.
        /// Users are intended to call this from InitPlugin() most of the time

        std::lock_guard<std::mutex> lock(mutex);
        auto svc = std::dynamic_pointer_cast<JService>(t);
        assert(svc != nullptr);
        underlying[std::type_index(typeid(T))] = std::make_pair(svc, new std::once_flag());
    }

    template<typename T>
    std::shared_ptr<T> get() {
        /// Retrieve a JService. If acquire_services() has not yet been called, it will be.
        /// Usually called from Service::finalize(). It may be called from anywhere,
        /// but it is generally safer to retrieve the services we need during acquire_dependencies()
        /// and keep pointers to those, instead of keeping a pointer to the ServiceLocator itself.
        /// (This also makes it easier to migrate to dependency injection if we so desire)

        auto iter = underlying.find(std::type_index(typeid(T)));
        if (iter == underlying.end()) {
            throw JException("Service not found!");
        }
        auto& pair = iter->second;
        auto& wired = pair.second;
        auto ptr = pair.first;
        std::call_once(*wired, [&](){ptr->acquire_services(this);});
        auto svc = std::static_pointer_cast<T>(ptr);
        return svc;
    }

    void wire_everything() {
        /// Make sure that all Services have been finalized. This is not strictly necessary,
        /// but it makes user errors easier to understand, and it prevents Services from being
        /// unpredictably finalized later on, particularly during computation.

        for (auto& item : underlying) {
            auto sharedptr = item.second.first;
            auto& wired = item.second.second;
            std::call_once(*wired, [&](){sharedptr->acquire_services(this);});
        }
    }
};

#endif // _JSERVICELOCATOR_H_
