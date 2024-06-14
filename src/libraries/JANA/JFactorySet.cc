
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iterator>
#include <iostream>

#include "JApplication.h"
#include "JFactorySet.h"
#include "JFactory.h"
#include "JFactoryGenerator.h"

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet() {
}

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(const std::vector<JFactoryGenerator*>& generators) {
    for (auto generator : generators) {
        generator->GenerateFactories(this);
    }
}

//---------------------------------
// ~JFactorySet    (Destructor)
//---------------------------------
JFactorySet::~JFactorySet() {

    // JFactories are owned by the JFactorySet.
    // JCollections are owned by their JFactory, unless no JFactory exists,
    // in which case collections are owned by the JFactorySet.

    for (auto& pair : mCollections) {
        if (pair->second.second == nullptr) {
            delete pair->second.first;
        }
    }
    for (auto& f : mFactories) { 
        delete f.second; 
    }
}


//---------------------------------
// Add
//---------------------------------
void JFactorySet::Add(JCollection* collection) {

    const auto& name = collection->GetCollectionName();
    auto it = mCollections.find(name);
    if (it != mCollections.end()) {
        // Collection is already present
        throw JException("Add(JCollection*) failed because collection '%s' is already present."
                         "Caller should first test using GetCollection()", name.c_str());
    }
    mCollections[name] = {collection, nullptr};
}


//---------------------------------
// Add
//---------------------------------
bool JFactorySet::Add(JFactory* factory) {
    /// Add a JFactory to this JFactorySet. This is usually called by the user from 
    /// JFactoryGenerator::GenerateFactories(). The JFactorySet assumes ownership of the
    /// factory.
    /// - If multiple factories produce the same collection, this throws an error.
    /// - If the same factory is added multiple times (e.g. by different plugins),
    ///   this prints a warning message but does NOT throw an error. The new factory 
    ///   will be immediately deleted.


    JFactory* duplicate_factory = nullptr;

    for (JCollection* coll : factory->GetCollections()) {

        auto name = coll->GetCollectionName();
        auto it = mCollections.find(name);
        if (it != mCollections.end()) {
            // Already in index
            if (factory->GetTypeName() == it->second->GetTypeName()) {
                // Found duplicate version of the same factory
                duplicate_factory = it->second;
                break;
            }
            else {
                throw JException("Two different factories are producing the same collection: '%s', '%s'", 
                        factory->GetTypeName().c_str(), 
                        it->second->GetTypeName().c_str());
            }
        }
        else {
            // Not in index, so we insert it and continue
            mCollections[name] = {coll, factory};
        }
    }

    if (duplicate_factory != nullptr) {
        JLogMessage(default_cout_logger, JLogger::Level::WARN) 
            << "Factory '" << factory->GetTypeName() << "' is being added twice. Plugins are: "
            << factory->GetPluginName() << ", " << duplicate_factory->GetPluginName() 
            << LOG_END;

        delete factory;
        return false;
    }
    else {
        mFactories.push_back(factory);
        return true;
    }
}


//---------------------------------
// GetCollection
//---------------------------------
std::pair<JCollection*, JFactory*> JFactorySet::GetCollection(std::string collection_name) const {
    /// If collection is absent, returns {nullptr, nullptr}
    /// If collection is present and factory is present, returns {collection, factory}
    /// If collection is present but factory is not, returns {collection, nullptr}
    /// If collection lives at the wrong event level, throw an exception

    auto it = mCollections.find(collection_name);
    if (it == std::end(mCollections)) {
        return {nullptr, nullptr};
    }
    JEventLevel found_level = it->second->GetLevel();
    if (found_level != mLevel) {
        throw JException("Collection belongs to a different level on the event hierarchy. Expected: %s, Found: %s", 
                toString(mLevel).c_str(), toString(found_level).c_str());
    }
    return it->second;
}


//---------------------------------
// GetAllCollections
//---------------------------------
std::vector<JCollection*> JFactorySet::GetAllCollections() const {
    /// Returns all JCollections that live at this event level

    std::vector<JCollection*> results;
    for (auto& pair : mCollections) {
        if (pair.second.second->GetEventLevel() == mLevel) {
            results.push_back(pair.second.first);
        }
    }
    return results;
}



//---------------------------------
// GetAllFactories
//---------------------------------
std::vector<JFactory*> JFactorySet::GetAllFactories() const {
    /// Returns all JFactories that live at this event level

    std::vector<JFactory*> results;
    for (JFactory* fac : mFactories) {
        if (fac->GetLevel() == mLevel) {
            results.push_back(fac);
        }
    }
    return results;
}

//---------------------------------
// GetAllMultifactories
//---------------------------------
std::vector<JMultifactory*> JFactorySet::GetAllMultifactories() const {
    std::vector<JMultifactory*> results;
    for (auto f : mMultifactories) {
        results.push_back(f);
    }
    return results;
}

/// Summarize() generates a JFactorySummary data object describing each JFactory
/// that this JFactorySet contains. The data is extracted from the JFactory itself.
/// Note that this includes ALL event levels, unlike GetAllCollections() and GetAllFactories().
std::vector<JFactorySummary> JFactorySet::Summarize() const {

    std::vector<JFactorySummary> results;
    for (auto& pair : mFactories) {
        results.push_back({
            .level = pair.second->GetLevel(),
            .plugin_name = pair.second->GetPluginName(),
            .factory_name = pair.second->GetFactoryName(),
            .factory_tag = pair.second->GetTag(),
            .object_name = pair.second->GetObjectName()
        });
    }
    return results;
}

//---------------------------------
// Print
//---------------------------------
void JFactorySet::Print() const
{
    size_t max_len = 0;
    for (auto p: mFactories) {
        if (p.second->GetLevel() != mLevel) continue;
        auto len = p.second->GetObjectName().length();
        if( len > max_len ) max_len = len;
    }

    max_len += 4;
    for (auto p: mFactories) {
        if (p.second->GetLevel() != mLevel) continue;
        auto name = p.second->GetObjectName();
        auto tag = p.second->GetTag();

        std::cout << std::string( max_len-name.length(), ' ') + name;
        if (!tag.empty()) std::cout << ":" << tag;
        std::cout << std::endl;
    }
}

/// Release() loops over all contained factories, clearing their data
void JFactorySet::Release() {

    for (const auto& pair : mCollections) {
        auto coll = pair->second.first;
        coll->ClearData();
    }
}
