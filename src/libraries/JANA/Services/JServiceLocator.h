
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JSERVICELOCATOR_H_
#define _JSERVICELOCATOR_H_


#include <map>
#include <typeinfo>
#include <typeindex>
#include <assert.h>
#include <memory>
#include <mutex>

#include <JANA/JException.h>
#include "JANA/Utils/JTypeInfo.h"
#include <JANA/JServiceFwd.h>



/// JService is a trait indicating that an object can be shared among JANA components
/// via a simple ServiceLocator. It provides a callback interface for configuring itself
/// when it depends on other JServices.

/// JServiceLocator is a nexus for collecting, initializing, and retrieving JServices.
/// This may be exposed via the JApplication facade, or used on its own. JServiceLocator
/// uses shared-pointer semantics to ensure that JServices always get freed,
/// but also don't get freed prematurely if a JApplication or JServiceLocator go out of scope.
class JServiceLocator {

    std::map<std::type_index, std::shared_ptr<JService>> underlying;
    std::mutex mutex;

public:


    template<typename T>
    void provide(std::shared_ptr<T> t) {

        /// Publish a Service to the ServiceLocator. This Service should have
        /// already been constructed, but not finalized.
        /// Users are intended to call this from InitPlugin() most of the time

        std::lock_guard<std::mutex> lock(mutex);
        auto svc = std::dynamic_pointer_cast<JService>(t);
        assert(svc != nullptr);
        underlying[std::type_index(typeid(T))] = svc;
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
            std::ostringstream oss;
            oss << "Service not found: '" << JTypeInfo::demangle<T>() << "'. Did you forget to include a plugin?" << std::endl;
            throw JException(oss.str());
        }
        auto svc = iter->second;
        svc->DoInit(this); 
        // Will short-circuit if already initialized
        // Note: Requires JApplication to already be set.

        auto svc_typed = std::static_pointer_cast<T>(svc);
        return svc_typed;
    }

    void wire_everything() {
        /// Make sure that all Services have been finalized. This is not strictly necessary,
        /// but it makes user errors easier to understand, and it prevents Services from being
        /// unpredictably finalized later on, particularly during computation.

        for (auto& entry : underlying) {
            entry.second->DoInit(this); 
        }
    }
};

#endif // _JSERVICELOCATOR_H_
