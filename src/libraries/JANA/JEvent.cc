
#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>


JEvent::JEvent() : mInspector(this){
}

JEvent::JEvent(JApplication* app) : mInspector(this) {
    // Furnish the JEvent with the parameter values and factory generators provided to the JApplication
    app->Initialize();
    app->GetService<JComponentManager>()->configure_event(*this);
}

JEvent::~JEvent() {
    mFactorySet.Clear();
}

/// GetFactory() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
JFactory* JEvent::GetFactory(const std::string& object_name, const std::string& tag) const {
    auto databundle = mFactorySet.GetDatabundle(object_name, tag);
    if (databundle != nullptr) {
        return databundle->GetFactory();
    }
    return nullptr;
}

/// GetAllFactories() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
std::vector<JFactory*> JEvent::GetAllFactories() const {
    return mFactorySet.GetAllFactories();
}

bool JEvent::HasParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return true;
    }
    return false;
}

const JEvent& JEvent::GetParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return *pair.second;
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
    mParents.push_back({level, parent});
    parent->mReferenceCount.fetch_add(1);
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
    auto remaining_refs = pair.second->mReferenceCount.fetch_sub(1);
    if (remaining_refs < 1) { // Remember, this was fetched _before_ the last subtraction
        throw JException("Parent refcount has gone negative!");
    }
    if (remaining_refs == 1) {
        return pair.second; 
        // Parent is no longer shared. Transfer back to arrow
    }
    else {
        return nullptr; // Parent is still shared by other children
    }
}

std::vector<JEvent*> JEvent::ReleaseAllParents() {
    std::vector<JEvent*> released_parents;

    for (auto it : mParents) {
        auto remaining_refs = it.second->mReferenceCount.fetch_sub(1);
        if (remaining_refs == 1) {
            released_parents.push_back(it.second);
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
    mFactorySet.Clear();
    mInspector.Reset();
    mCallGraph.Reset();
}

void JEvent::Finish() {
    mFactorySet.Finish();
}


