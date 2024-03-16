
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iterator>
#include <iostream>

#include "JApplication.h"
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
JFactorySet::JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators)
{
    //Add all factories from all factory generators
    for(auto sGenerator : aFactoryGenerators){

        // Generate the factories into a temporary JFactorySet.
        JFactorySet myset;
        sGenerator->GenerateFactories( &myset );

        // Merge factories from temporary JFactorySet into this one. Any that
        // already exist here will leave the duplicates in the temporary set
        // where they will be destroyed by its destructor as it falls out of scope.
        Merge( myset );
    }
}

JFactorySet::JFactorySet(JFactoryGenerator* source_gen, const std::vector<JFactoryGenerator*>& default_gens) {

    if (source_gen != nullptr) source_gen->GenerateFactories(this);
    for (auto gen : default_gens) {
        JFactorySet temp_set;
        gen->GenerateFactories(&temp_set);
        Merge(temp_set); // Factories which are shadowed stay in temp_set; others are removed
        for (auto straggler : temp_set.GetAllFactories()) {
            LOG << "Factory '" << straggler->GetFactoryName() << "' overriden, will be excluded from event." << LOG_END;
        }
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
        for (auto& f : mFactories) delete f.second;
    }
    // Now that the factories are deleted, nothing can call the multifactories so it is safe to delete them as well
    for (auto* mf : mMultifactories) { delete mf; }


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

    auto typed_key = std::make_pair( aFactory->GetObjectType(), aFactory->GetTag() );
    auto untyped_key = std::make_pair( aFactory->GetObjectName(), aFactory->GetTag() );

    auto typed_result = mFactories.find(typed_key);
    auto untyped_result = mFactoriesFromString.find(untyped_key);

    if (typed_result != std::end(mFactories) || untyped_result != std::end(mFactoriesFromString)) {
        // Factory is duplicate. Since this almost certainly indicates a user error, and
        // the caller will not be able to do anything about it anyway, throw an exception.
        throw JException("JFactorySet::Add failed because factory is duplicate");
        // return false;
    }

    mFactories[typed_key] = aFactory;
    mFactoriesFromString[untyped_key] = aFactory;
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
// GetAllFactories
//---------------------------------
std::vector<JFactory*> JFactorySet::GetAllFactories() const {
    std::vector<JFactory*> results;
    for (auto p : mFactories) {
        if (p.second->GetLevel() == mLevel) {
            results.push_back(p.second);
        }
    }
    return results;
}


//---------------------------------
// Merge
//---------------------------------
void JFactorySet::Merge(JFactorySet &aFactorySet)
{
    /// Merge any factories in the specified JFactorySet into this
    /// one. Any factories which don't have the same type and tag as one
    /// already in this set will be transferred and this JFactorySet
    /// will take ownership of them. Ones that have a type and tag
    /// that matches one already in this set will be left in the
    /// original JFactorySet. Thus, all factories left in the JFactorySet
    /// passed into this method upon return from it can be considered
    /// duplicates. It will be left to the caller to delete those.

    JFactorySet tmpSet; // keep track of duplicates to copy back into aFactorySet
    for( auto pair : aFactorySet.mFactories ){
        auto factory = pair.second;

        auto typed_key = std::make_pair(factory->GetObjectType(), factory->GetTag());
        auto untyped_key = std::make_pair(factory->GetObjectName(), factory->GetTag());

        auto typed_result = mFactories.find(typed_key);
        auto untyped_result = mFactoriesFromString.find(untyped_key);

        if (typed_result != std::end(mFactories) || untyped_result != std::end(mFactoriesFromString)) {
            // Factory is duplicate. Return to caller just in case
            tmpSet.mFactories[pair.first] = factory;
        }
        else {
            mFactories[typed_key] = factory;
            mFactoriesFromString[untyped_key] = factory;
        }
    }

    // Copy duplicates back to aFactorySet
    aFactorySet.mFactories.swap( tmpSet.mFactories );
    tmpSet.mFactories.clear(); // prevent ~JFactorySet from deleting any factories

    // Move ownership of multifactory pointers over.
    for (auto* mf : aFactorySet.mMultifactories) {
        mMultifactories.push_back(mf);
    }
    aFactorySet.mMultifactories.clear();
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

    for (const auto& sFactoryPair : mFactories) {
        auto sFactory = sFactoryPair.second;
        sFactory->ClearData();
    }
}

/// Summarize() generates a JFactorySummary data object describing each JFactory
/// that this JFactorySet contains. The data is extracted from the JFactory itself.
std::vector<JFactorySummary> JFactorySet::Summarize() const {

    std::vector<JFactorySummary> results;
    for (auto& pair : mFactories) {
        if (pair.second->GetLevel() != mLevel) continue;
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
