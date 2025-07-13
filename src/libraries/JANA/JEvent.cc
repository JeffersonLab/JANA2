
#include "JANA/Utils/JEventLevel.h"
#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>
#include <cstdint>
#include <sstream>


JEvent::JEvent() : mInspector(this){
    mFactorySet = new JFactorySet();
}

JEvent::JEvent(JApplication* app) : mInspector(this) {
    // Furnish the JEvent with the parameter values and factory generators provided to the JApplication
    app->Initialize();
    app->GetService<JComponentManager>()->configure_event(*this);
}

JEvent::~JEvent() {
    if (mFactorySet != nullptr) {
        // Prevent memory leaks of factory contents
        mFactorySet->Clear();
        // We mustn't call EndRun() or Finish() here because that would give us an excepting destructor
    }
    delete mFactorySet;
}

void JEvent::SetFactorySet(JFactorySet* factorySet) {
    delete mFactorySet;
    mFactorySet = factorySet;
#if JANA2_HAVE_PODIO
    // Maintain the index of PODIO factories
    for (JFactory* factory : mFactorySet->GetAllFactories()) {
        if (dynamic_cast<JFactoryPodio*>(factory) != nullptr) {
            auto tag = factory->GetTag();
            auto it = mPodioFactories.find(tag);
            if (it != mPodioFactories.end()) {
                throw JException("SetFactorySet failed because PODIO factory tag '%s' is not unique", tag.c_str());
            }
            mPodioFactories[tag] = factory;
        }
    }
#endif
}


/// GetFactory() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
JFactory* JEvent::GetFactory(const std::string& object_name, const std::string& tag) const {
    return mFactorySet->GetFactory(object_name, tag);
}

/// GetAllFactories() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
std::vector<JFactory*> JEvent::GetAllFactories() const {
    return mFactorySet->GetAllFactories();
}

bool JEvent::HasParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return true;
    }
    return false;
}

const JEvent& JEvent::GetParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return *pair.second.first;
    }
    throw JException("Unable to find parent at level %s", 
                        toString(level).c_str());
}

void JEvent::SetParent(JEvent* parent) {
    JEventLevel level = parent->GetLevel();
    for (const auto& pair : mParents) {
        if (pair.first == level) throw JException("Event already has a parent at level %s", 
                                                    toString(parent->GetLevel()).c_str());
    }
    mParents.push_back({level, {parent, parent->GetEventNumber()}});
    parent->mReferenceCount.fetch_add(1);
    MakeEventStamp();
}


void JEvent::MakeEventStamp() const {
    std::ostringstream ss;
    ss << toChar(mFactorySet->GetLevel()) << ":" << mEventNumber;
    if (!mParents.empty()) {
        ss << "(";
        size_t parent_count = mParents.size();
        for (size_t i=0; i<parent_count; ++i) {
            auto parent = mParents[i].second;
            ss << parent.first->GetEventStamp();
            if (i != parent_count-1) {
                ss << ",";
            }
        }
        ss << ")";
    }
    mEventStamp = ss.str();
};

const std::string& JEvent::GetEventStamp() const {
    if (mEventStamp.empty()) {
        MakeEventStamp();
    }
    return mEventStamp;
}

void JEvent::SetParentNumber(JEventLevel level, uint64_t number) {
    for (const auto& pair : mParents) {
        if (pair.first == level) {
            throw JException("Event already has a parent at level %s", toString(level).c_str());
        }
    }
    mParents.push_back({level, {nullptr, number}});
    MakeEventStamp();
}


uint64_t JEvent::GetParentNumber(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) {
            return pair.second.second;
        }
    }
    return 0;
}


JEvent* JEvent::ReleaseParent(JEventLevel level) {
    if (mParents.size() == 0) {
        throw JException("ReleaseParent failed: child has no parents!");
    }
    auto pair = mParents.back();
    if (pair.first != level) {
        throw JException("JEvent::ReleaseParent called out of level order: Caller expected %s, but parent was actually %s", 
                toString(level).c_str(), toString(pair.first).c_str());
    }
    mParents.pop_back();
    auto remaining_refs = pair.second.first->mReferenceCount.fetch_sub(1);
    if (remaining_refs < 1) { // Remember, this was fetched _before_ the last subtraction
        throw JException("Parent refcount has gone negative!");
    }
    if (remaining_refs == 1) {
        return pair.second.first; 
        // Parent is no longer shared. Transfer back to arrow
    }
    else {
        return nullptr; // Parent is still shared by other children
    }
}

std::vector<JEvent*> JEvent::ReleaseAllParents() {
    std::vector<JEvent*> released_parents;

    for (auto it : mParents) {
        auto remaining_refs = it.second.first->mReferenceCount.fetch_sub(1);
        if (remaining_refs == 1) {
            released_parents.push_back(it.second.first);
        }
    }
    mParents.clear();
    return released_parents;
}

void JEvent::TakeRefToSelf() {
    mReferenceCount++;
}

int JEvent::ReleaseRefToSelf() {
    int remaining_refs = mReferenceCount.fetch_sub(1);
    remaining_refs -= 1; // fetch_sub post increments
    if (remaining_refs < 0) {
        throw JException("JEvent's own refcount has gone negative!");
    }
    return remaining_refs;
}

int JEvent::GetChildCount() {
    return mReferenceCount;
}

void JEvent::Clear(bool processed_successfully) {
    if (processed_successfully && mEventSource != nullptr) {
        mEventSource->DoFinishEvent(*this);
        mIsWarmedUp = true;
    }
    mFactorySet->Clear();
    mInspector.Reset();
    mCallGraph.Reset();
}

void JEvent::Finish() {
    mFactorySet->Finish();
}


