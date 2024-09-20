
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iterator>
#include <iostream>
#include <typeindex>
#include <vector>

#include <JANA/Components/JStorage.h>
#include "JFactorySet.h"
#include "JFactory.h"
#include "JMultifactory.h"
#include "JFactoryGenerator.h"

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(void)
{

}

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(const std::vector<JFactoryGenerator*>& generators)
{
    // Add all factories from all factory generators
    for(auto generator : generators){
        generator->GenerateFactories(this);
    }
}

//---------------------------------
// ~JFactorySet    (Destructor)
//---------------------------------
JFactorySet::~JFactorySet()
{
    /// The destructor will delete any factories in the set, unless mIsFactoryOwner is set to false.
    /// The only time mIsFactoryOwner should/can be set false is when a JMultifactory is using a JFactorySet internally
    /// to manage its JMultifactoryHelpers.
    if (mIsFactoryOwner) {
        for (auto& s : mCollectionsFromName) {
            // Only delete _inserted_ collections. Collections are otherwise owned by their factories
            if (s.second->GetFactory() == nullptr) {
                delete s.second;
            }
        }
        for (auto f : mAllFactories) delete f;
        // Now that the factories are deleted, nothing can call the multifactories so it is safe to delete them as well

        for (auto* mf : mMultifactories) { delete mf; }
    }
}

//---------------------------------
// Add
//---------------------------------
void JFactorySet::Add(JStorage* collection) {

    if (collection->GetCollectionName().empty()) {
        throw JException("Attempted to add a collection with no name");
    }
    auto named_result = mCollectionsFromName.find(collection->GetCollectionName());
    if (named_result != std::end(mCollectionsFromName)) {
        // Collection is duplicate. Since this almost certainly indicates a user error, and
        // the caller will not be able to do anything about it anyway, throw an exception.
        // We show the user which factory is causing this problem, including both plugin names

        auto ex = JException("Attempted to add duplicate collections");
        ex.function_name = "JFactorySet::Add";
        ex.instance_name = collection->GetCollectionName();

        auto fac = collection->GetFactory();
        if (fac != nullptr) {
            ex.type_name = fac->GetTypeName();
            ex.plugin_name = fac->GetPluginName();
            if (named_result->second->GetFactory() != nullptr) {
                ex.plugin_name += ", " + named_result->second->GetFactory()->GetPluginName();
            }
        }
        throw ex;
    }
    // Note that this is agnostic to event level. We may decide to change this.
    mCollectionsFromName[collection->GetCollectionName()] = collection;
}

//---------------------------------
// Add
//---------------------------------
bool JFactorySet::Add(JFactory* aFactory)
{
    /// Add a JFactory to this JFactorySet. The JFactorySet assumes ownership of this factory.
    /// If the JFactorySet already contains a JFactory with the same key,
    /// throw an exception and let the user figure out what to do.
    /// This scenario occurs when the user has multiple JFactory<T> producing the
    /// same T JObject, and is not distinguishing between them via tags.
    
    // There are two different ways JFactories can work now. In the old way, JFactory must be
    // a JFactoryT, and have exactly one output collection. In the new way (which includes JFactoryPodioT), 
    // JFactory has an arbitrary number of output collections which are explicitly
    // represented, similar to but better than JMultifactory. We distinguish between
    // these two cases by checking whether JFactory::GetObjectType returns an object type vs nullopt.


    mAllFactories.push_back(aFactory);

    if (aFactory->GetOutputs().empty()) {
        // We have an old-style JFactory!

        auto typed_key = std::make_pair( aFactory->GetObjectType(), aFactory->GetTag() );
        auto untyped_key = std::make_pair( aFactory->GetObjectName(), aFactory->GetTag() );

        auto typed_result = mFactories.find(typed_key);
        auto untyped_result = mFactoriesFromString.find(untyped_key);

        if (typed_result != std::end(mFactories) || untyped_result != std::end(mFactoriesFromString)) {
            // Factory is duplicate. Since this almost certainly indicates a user error, and
            // the caller will not be able to do anything about it anyway, throw an exception.
            // We show the user which factory is causing this problem, including both plugin names
            std::string other_plugin_name;
            if (typed_result != std::end(mFactories)) {
                other_plugin_name = typed_result->second->GetPluginName();
            }
            else {
                other_plugin_name = untyped_result->second->GetPluginName();
            }
            auto ex = JException("Attempted to add duplicate factories");
            ex.function_name = "JFactorySet::Add";
            ex.instance_name = aFactory->GetPrefix();
            ex.type_name = aFactory->GetTypeName();
            ex.plugin_name = aFactory->GetPluginName() + ", " + other_plugin_name;
            throw ex;
        }

        mFactories[typed_key] = aFactory;
        mFactoriesFromString[untyped_key] = aFactory;
    }
    else {
        // We have a new-style JFactory!
        for (const auto* output : aFactory->GetOutputs()) {
            for (const auto& coll : output->GetCollections()) {
                coll->SetFactory(aFactory);
                Add(coll.get());
            }
        }
    }
    return true;
}


bool JFactorySet::Add(JMultifactory *multifactory) {
    /// Add a JMultifactory to this JFactorySet. This JFactorySet takes ownership of its JMultifactoryHelpers,
    /// which was previously held by the JMultifactory.mHelpers JFactorySet.
    /// Ownership of the JMultifactory itself is shared among those helpers.

    auto helpers = multifactory->GetHelpers();
    for (auto fac : helpers->GetAllFactories()) {
        Add(fac);
    }
    helpers->mIsFactoryOwner = false;
    mMultifactories.push_back(multifactory);
    /// This is a little bit weird, but we are using a JFactorySet internally to JMultifactory in order to store and
    /// efficiently access its JMultifactoryHelpers. Ownership of the JMultifactoryHelpers is transferred to
    /// the enclosing JFactorySet.
    return true;
}

//---------------------------------
// GetCollection
//---------------------------------
JStorage* JFactorySet::GetStorage(const std::string& collection_name) const {
    auto it = mCollectionsFromName.find(collection_name);
    if (it != std::end(mCollectionsFromName)) {
        auto fac = it->second->GetFactory();
        if (fac != nullptr && fac->GetLevel() != mLevel) {
            throw JException("Collection belongs to a different level on the event hierarchy!");
        }
        return it->second;
    }
    return nullptr;
}


//---------------------------------
// GetFactory
//---------------------------------
JFactory* JFactorySet::GetFactory(const std::string& object_name, const std::string& tag) const
{
    auto untyped_key = std::make_pair(object_name, tag);
    auto it = mFactoriesFromString.find(untyped_key);
    if (it != std::end(mFactoriesFromString)) {
        if (it->second->GetLevel() != mLevel) {
            throw JException("Factory belongs to a different level on the event hierarchy!");
        }
        return it->second;
    }
    return nullptr;
}

//---------------------------------
// GetFactory
//---------------------------------
JFactory* JFactorySet::GetFactory(std::type_index object_type, const std::string& object_name, const std::string& tag) const {

    auto typed_key = std::make_pair(object_type, tag);
    auto typed_iter = mFactories.find(typed_key);
    if (typed_iter != std::end(mFactories)) {
        JEventLevel found_level = typed_iter->second->GetLevel();
        if (found_level != mLevel) {
            throw JException("Factory belongs to a different level on the event hierarchy. Expected: %s, Found: %s", toString(mLevel).c_str(), toString(found_level).c_str());
        }
        return typed_iter->second;
    }
    return GetFactory(object_name, tag);
}

//---------------------------------
// GetAllFactories
//---------------------------------
std::vector<JFactory*> JFactorySet::GetAllFactories() const {

    // This returns both old-style (JFactoryT) and new-style (JFactory+JStorage) factories, unlike 
    // GetAllFactories(object_type, object_name) below. This is because we use this method in 
    // JEventProcessors to activate factories in a generic way, particularly when working with Podio data.

    return mAllFactories;
}

//---------------------------------
// GetAllFactories
//---------------------------------
std::vector<JFactory*> JFactorySet::GetAllFactories(std::type_index object_type, const std::string& object_name) const {

    // This returns all factories which _directly_ produce objects of type object_type, i.e. they don't use a JStorage.
    // This is what all of its callers already expect anyhow. Obviously we'd like to migrate everything over to JStorage
    // eventually. Rather than updating this to also check mCollectionsFromName, it probably makes more sense to create 
    // a JFactorySet::GetAllStorages(type_index, object_name) instead, and migrate all callers to use that.

    std::vector<JFactory*> results;
    for (auto& it : mFactories) {
        if (it.second->GetObjectType() == object_type || it.second->GetObjectName() == object_name) {
            results.push_back(it.second);
        }
    }
    return results;
}

//---------------------------------
// GetAllMultifactories
//---------------------------------
std::vector<JMultifactory*> JFactorySet::GetAllMultifactories() const {
    return mMultifactories;
}

//---------------------------------
// GetAllCollectionNames
//---------------------------------
std::vector<std::string> JFactorySet::GetAllCollectionNames() const {
    std::vector<std::string> results;
    for (const auto& it : mCollectionsFromName) {
        results.push_back(it.first);
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
    for (auto* fac : mAllFactories) {
        fac->ClearData();
    }
    for (auto& it : mCollectionsFromName) {
        if (it.second->GetFactory() == nullptr) {
            it.second->ClearData();
        }
    }
}

