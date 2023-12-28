
#pragma once

#include "JOmniFactory.h"

#include <map>

template <typename LevelT>
struct HasFactories {

    std::vector<std::unique_ptr<JOmniFactory<LevelT>>> factories;
    std::map<std::string, std::pair<JCollection*, JOmniFactory<LevelT>*>> collection_index;

    ~HasFactories() {
        for (auto& item : collection_index) {
            if (item.second.second == nullptr) {
                // If collection is not owned by a factory -- in other words, if it is something we inserted, 
                // then go ahead and delete it. Otherwise the collection is owned by its factory.
                delete item.second.first;
            }
        }
    }

    void AddFactory(std::unique_ptr<JOmniFactory<LevelT>> factory) {
        for (OutputBase* output : factory->outputs) {
            JCollection* collection = output->GetCollection();
            if (collection->is_prefixed) {
                collection_index.insert({collection->type_name + ":" + collection->collection_name, {collection, factory.get()}});
            }
            else {
                collection_index.insert({collection->collection_name, {collection, factory.get()}});
            }
        }
        factories.push_back(std::move(factory));
    }

    void Insert(JCollection* collection) {
        // TODO: Communicate ownership transfer using unique_ptr or shared_ptr
        if (collection->is_prefixed) {
            collection_index.insert({collection->type_name + ":" + collection->collection_name, {collection, nullptr}});
        }
        else {
            collection_index.insert({collection->collection_name, {collection, nullptr}});
        }
    }


    template <typename CollT = JCollection>
    CollT* GetOrCreate(std::string collection_name) {
        auto entry = collection_index.find(collection_name);
        if (entry == collection_index.end()) {
            throw JException("No collection named '%s' found at level '%s'>", collection_name.c_str(), JTypeInfo::demangle<LevelT>().c_str());
        }
        JCollection* collection = entry->second.first;
        JOmniFactory<LevelT>* factory = entry->second.second;
        if (factory != nullptr) {
            LevelT* event = static_cast<LevelT*>(this);
            factory->DoProcess(*event);
        }
        CollT* typed_collection = dynamic_cast<CollT*>(collection);
        if (typed_collection == nullptr) {
            throw JException("Unable to cast collection '%s' to %s*", collection_name.c_str(), JTypeInfo::demangle<CollT>().c_str());
        }
        return typed_collection;
    }

    template <typename CollT = JCollection>
    CollT* Get(std::string collection_name) {
        auto entry = collection_index.find(collection_name);
        if (entry == collection_index.end()) {
            throw JException("No collection named '%s' found at level '%s'", collection_name.c_str(), JTypeInfo::demangle<LevelT>().c_str());
        }
        JCollection* collection = entry->second.first;
        JOmniFactory<LevelT>* factory = entry->second.second;
        CollT* typed_collection = dynamic_cast<CollT*>(collection);
        if (typed_collection == nullptr) {
            throw JException("Unable to cast collection '%s' to %s*", collection_name.c_str(), JTypeInfo::demangle<CollT>().c_str());
        }
        return typed_collection;
    }

    template <typename DataT>
    void Create(std::string collection_name) {
        auto entry = collection_index.find(collection_name);
        if (entry == collection_index.end()) {
            throw JException("No collection named '%s' found in JOmniFactorySet<%s>", collection_name.c_str(), JTypeInfo::demangle<LevelT>().c_str());
        }
        JOmniFactory<LevelT>* factory = entry->second.second;
        if (factory != nullptr) {
            LevelT* event = static_cast<LevelT*>(this);
            factory->DoProcess(*event);
        }
    }

    void Reset() {
        // Clear all collections 
        // Reset flags on all factories?
    }
};

