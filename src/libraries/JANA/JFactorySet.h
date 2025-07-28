
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <string>
#include <typeindex>
#include <map>

#include <JANA/Components/JComponentSummary.h>
#include <JANA/Components/JDatabundle.h>
#include <JANA/Utils/JEventLevel.h>

class JFactory;

class JFactorySet {

private:
    JEventLevel mLevel = JEventLevel::PhysicsEvent;
    std::vector<JFactory*> mFactories;
    std::vector<JDatabundle*> mDatabundles;

    std::map<std::string, JDatabundle*> mDatabundleFromUniqueName;
    std::map<std::pair<std::type_index, std::string>, JDatabundle*> mDatabundleFromTypeIndexAndEitherName;
    std::map<std::pair<std::string, std::string>, JDatabundle*> mDatabundleFromTypeNameAndEitherName;

    std::map<std::type_index, std::vector<JDatabundle*>> mDatabundlesFromTypeIndex;
    std::map<std::string, std::vector<JDatabundle*>> mDatabundlesFromTypeName;

public:
    JFactorySet();
    virtual ~JFactorySet();

    bool Add(JFactory* aFactory);
    void Add(JDatabundle* databundle);
    void Print() const;
    void Clear();
    void Finish();

    JEventLevel GetLevel() const { return mLevel; }
    void SetLevel(JEventLevel level) { mLevel = level; }

    const std::vector<JFactory*>& GetAllFactories() const { return mFactories; }
    const std::vector<JDatabundle*>& GetAllDatabundles() const { return mDatabundles; }

    JDatabundle* GetDatabundle(const std::string& unique_name) const;
    JDatabundle* GetDatabundle(const std::string& object_type_name, const std::string& short_or_unique_name) const;
    JDatabundle* GetDatabundle(std::type_index object_type_index, const std::string& short_or_unique_name) const;

    const std::vector<JDatabundle*>& GetDatabundles(std::type_index index) const;
    const std::vector<JDatabundle*>& GetDatabundles(const std::string& object_type_name) const;

};



