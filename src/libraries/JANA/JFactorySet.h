
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JTypeInfo.h>

#include <vector>
#include <string>
#include <typeindex>
#include <map>

class JFactoryGenerator;
class JFactory;
class JMultifactory;
class JStorage;


class JFactorySet {

private:
    std::vector<JFactory*> mAllFactories;
    std::map<std::pair<std::type_index, std::string>, JFactory*> mFactories;        // {(typeid, tag) : factory}
    std::map<std::pair<std::string, std::string>, JFactory*> mFactoriesFromString;  // {(objname, tag) : factory}
    std::map<std::string, JStorage*> mCollectionsFromName;
    std::vector<JMultifactory*> mMultifactories;
    bool mIsFactoryOwner = true;
    JEventLevel mLevel = JEventLevel::PhysicsEvent;

public:
    JFactorySet();
    JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators);
    virtual ~JFactorySet();

    bool Add(JFactory* aFactory);
    bool Add(JMultifactory* multifactory);
    void Add(JStorage* storage);
    void Print() const;
    void Release();

    std::vector<std::string> GetAllCollectionNames() const;
    JStorage* GetStorage(const std::string& collection_name) const;

    JFactory* GetFactory(const std::string& object_name, const std::string& tag="") const;
    JFactory* GetFactory(std::type_index object_type, const std::string& object_name, const std::string& tag = "") const;

    std::vector<JFactory*> GetAllFactories() const;
    std::vector<JFactory*> GetAllFactories(std::type_index object_type, const std::string& object_name) const;

    std::vector<JMultifactory*> GetAllMultifactories() const;

    JEventLevel GetLevel() const { return mLevel; }
    void SetLevel(JEventLevel level) { mLevel = level; }

    template <typename T>
    [[deprecated]]
    JFactory* GetFactory(const std::string& tag) const {
        auto object_name = JTypeInfo::demangle<T>();
        return GetFactory(object_name, tag);
    }
};



