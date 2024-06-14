// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JFactory.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JResettable.h>
#include <JANA/Status/JComponentSummary.h>

#include <string>
#include <vector>
#include <map>

class JFactoryGenerator;

/// JFactorySet contains all factories associated with a single JEvent (and consequently a single worker thread). 
/// These are indexed by the names of the collections that each JFactory provides. JFactorySet doesn't execute any
/// JFactories on its own. All it does is retrieve collections and their corresponding factories for JEvent, and provide 
/// sensible error handling in case the user provides multiple factories that can produce a particular collection, etc.

class JFactorySet : public JResettable {

protected:
    std::map<std::string, std::pair<JCollection*,JFactory*>> mCollections;    // {"objname:tag" : (collection, factory)}
    std::vector<JFactory*> mFactories;
    JEventLevel mLevel = JEventLevel::PhysicsEvent;

public:
    JFactorySet();
    JFactorySet(const std::vector<JFactoryGenerator*>& generators);
    virtual ~JFactorySet();
        std::vector<JMultifactory*> GetAllMultifactories() const;

    void Add(JCollection* collection);
    bool Add(JFactory* collection);

    std::pair<JCollection*, JFactory*> GetCollection(std::string collection_name) const;
    std::vector<JCollection*> GetAllCollections() const;
    std::vector<JFactory*> GetAllFactories() const;

    std::vector<JFactorySummary> Summarize() const;
    void Print() const;
    void Release();
    JEventLevel GetLevel() const { return mLevel; }
    void SetLevel(JEventLevel level) { mLevel = level; }

};

