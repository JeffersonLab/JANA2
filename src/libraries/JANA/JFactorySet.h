
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <string>
#include <typeindex>
#include <map>

#include <JANA/JFactoryT.h>
#include <JANA/Components/JPodioCollection.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Components/JComponentSummary.h>

class JFactoryGenerator;
class JFactory;
class JMultifactory;


class JFactorySet {

    protected:
        std::map<std::pair<std::type_index, std::string>, JFactory*> mFactories;        // {(typeid, tag) : factory}
        std::map<std::pair<std::string, std::string>, JFactory*> mFactoriesFromString;  // {(objname, tag) : factory}
        std::map<std::string, JCollection*> mCollectionsFromName;
        std::vector<JMultifactory*> mMultifactories;
        bool mIsFactoryOwner = true;
        JEventLevel mLevel = JEventLevel::PhysicsEvent;

    public:
        JFactorySet();
        JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators);
        virtual ~JFactorySet();

        bool Add(JFactory* aFactory);
        bool Add(JMultifactory* multifactory);
        void Print(void) const;
        void Release(void);

        JCollection* GetCollection(const std::string& collection_name) const;
        JFactory* GetFactory(const std::string& object_name, const std::string& tag="") const;
        template<typename T> JFactoryT<T>* GetFactory(const std::string& tag = "") const;
        std::vector<JFactory*> GetAllFactories() const;
        std::vector<JMultifactory*> GetAllMultifactories() const;
        template<typename T> std::vector<JFactoryT<T>*> GetAllFactories() const;

        JEventLevel GetLevel() const { return mLevel; }
        void SetLevel(JEventLevel level) { mLevel = level; }
};


template<typename T>
JFactoryT<T>* JFactorySet::GetFactory(const std::string& tag) const {

    auto typed_key = std::make_pair(std::type_index(typeid(T)), tag);
    auto typed_iter = mFactories.find(typed_key);
    if (typed_iter != std::end(mFactories)) {
        JEventLevel found_level = typed_iter->second->GetLevel();
        if (found_level != mLevel) {
            throw JException("Factory belongs to a different level on the event hierarchy. Expected: %s, Found: %s", toString(mLevel).c_str(), toString(found_level).c_str());
        }
        return static_cast<JFactoryT<T>*>(typed_iter->second);
    }

    auto untyped_key = std::make_pair(JTypeInfo::demangle<T>(), tag);
    auto untyped_iter = mFactoriesFromString.find(untyped_key);
    if (untyped_iter != std::end(mFactoriesFromString)) {
        JEventLevel found_level = untyped_iter->second->GetLevel();
        if (found_level != mLevel) {
            throw JException("Factory belongs to a different level on the event hierarchy. Expected: %s, Found: %s", toString(mLevel).c_str(), toString(found_level).c_str());
        }
        return static_cast<JFactoryT<T>*>(untyped_iter->second);
    }
    return nullptr;
}

template<typename T>
std::vector<JFactoryT<T>*> JFactorySet::GetAllFactories() const {
    auto sKey = std::type_index(typeid(T));
    std::vector<JFactoryT<T>*> data;
    for (auto it=std::begin(mFactories);it!=std::end(mFactories);it++){
        if (it->first.first==sKey){
            if (it->second->GetLevel() == mLevel) {
                data.push_back(static_cast<JFactoryT<T>*>(it->second));
            }
        }
    }
    return data;
}



