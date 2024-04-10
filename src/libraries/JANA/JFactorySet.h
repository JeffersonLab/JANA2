
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactorySet_h_
#define _JFactorySet_h_

#include <string>
#include <typeindex>
#include <map>

#include <JANA/JFactoryT.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JResettable.h>
#include <JANA/Status/JComponentSummary.h>

class JFactoryGenerator;
class JFactory;
class JMultifactory;


class JFactorySet : public JResettable
{
    public:
        JFactorySet(void);
        JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators);
        JFactorySet(JFactoryGenerator* source_gen, const std::vector<JFactoryGenerator*>& default_gens);
        virtual ~JFactorySet();

        bool Add(JFactory* aFactory);
        bool Add(JMultifactory* multifactory);
        void Merge(JFactorySet &aFactorySet);
        void Print(void) const;
        void Release(void);

        JFactory* GetFactory(const std::string& object_name, const std::string& tag="") const;
        template<typename T> JFactoryT<T>* GetFactory(const std::string& tag = "") const;
        std::vector<JFactory*> GetAllFactories() const;
        template<typename T> std::vector<JFactoryT<T>*> GetAllFactories() const;

        std::vector<JFactorySummary> Summarize() const;

        JEventLevel GetLevel() const { return mLevel; }
        void SetLevel(JEventLevel level) { mLevel = level; }

    protected:
        std::map<std::pair<std::type_index, std::string>, JFactory*> mFactories;        // {(typeid, tag) : factory}
        std::map<std::pair<std::string, std::string>, JFactory*> mFactoriesFromString;  // {(objname, tag) : factory}
        std::vector<JMultifactory*> mMultifactories;
        bool mIsFactoryOwner = true;
        JEventLevel mLevel = JEventLevel::Event;
        
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


#endif // _JFactorySet_h_

