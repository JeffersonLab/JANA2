//
// Created by Nathan W Brei on 2019-03-10.
//
#pragma once

#include <map>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <assert.h>
#include <chrono>

namespace greenfield {

    class ServiceLocator;

    struct Service {
        // InitPlugin() provides Services to the ServiceLocator.

        virtual void acquire_services(ServiceLocator *sl) {};
        // Do not ask for services from the ServiceLocator in the constructor,
        // because they might not have been provided yet. (We can't really control
        // the order in which plugins get loaded) Instead, put all code that retrieves
        // Services in acquire_services(). ServiceProvider will call this back once all of the
        // Services have been provided. Also, calling ServiceLocator::get() from anywhere
        // will recursively acquire_services for all Service dependencies.
    };

    class ServiceLocator {

        std::map<std::type_index, std::pair<Service*,bool>> underlying;

      public:

        template <typename T> void provide(T* t) {

            /// Publish a Service to the ServiceLocator. This Service should have
            /// already been constructed, but not finalized.
            /// Users are intended to call this from InitPlugin() most of the time

            auto svc = dynamic_cast<Service*>(t);
            assert(svc != nullptr);
            underlying[std::type_index(typeid(T))] = std::make_pair(svc, false);
        }

        template <typename T> T* get() {
            /// Retrieve a Service. If it has not yet been finalized, it will be.
            /// Usually called from Service::finalize(). It may be called from anywhere,
            /// but it is generally safer to retrieve the services we need during acquire_dependencies()
            /// and keep pointers to those, instead of keeping a pointer to the ServiceLocator itself.
            /// (This also makes it easier to migrate to dependency injection if we so desire)

            auto pair = underlying[std::type_index(typeid(T))];
            bool& wired = pair.second;
            auto ptr = pair.first;
            if (!wired) {
                ptr->acquire_services(this);
                wired = true;
            }
            auto svc = dynamic_cast<T*>(ptr);
            return svc;
        }

        void wire_everything() {
            /// Make sure that all Services have been finalized. This is not strictly necessary,
            /// but it makes user errors easier to understand, and it prevents Services from being
            /// unpredictably finalized later on, particularly during computation.

            for (auto& item : underlying) {
                Service* ptr = item.second.first;
                bool finalized = item.second.second;
                if (!finalized) {
                    ptr->acquire_services(this);
                }
            }
        }

        // TODO: Consider adding a finalize() callback which runs after all Services have been wired up,
        // similar to Spring framework's PostConstruct. This would give us three effective phases of initialization:
        // (1) Service construction; ServiceLocator::provide()
        // (2) Service::acquire_services(), which wires together all of our dependencies but doesn't use them
        // (3) Service::finalize(), where we can actually use our Service dependencies
        // This helps deal with problems of circular dependencies between Services, but doesn't 100% resolve them.

    };

    /// To be replaced with the real JParameterManager when the time is right
    struct ParameterManager : public Service {
        using duration_t = std::chrono::steady_clock::duration;

        int chunksize;
        int backoff_tries;
        duration_t backoff_time;
        duration_t checkin_time;


    };

    // Global variable which serves as a replacement for japp
    // Ideally we would make this be a singleton instead, but until we figure out
    // how to merge our plugins' symbol tables correctly, singletons are broken.
    extern ServiceLocator* serviceLocator;

}
