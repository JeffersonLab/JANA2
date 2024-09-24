
#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>

JEvent::JEvent(JApplication* app) : mInspector(&(*this)) {
    if (app != nullptr) {
        app->GetService<JComponentManager>()->configure_event(*this);
    }
    else {
        mFactorySet = new JFactorySet();
    }
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

bool JEvent::HasParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return true;
    }
    return false;
}

const JEvent& JEvent::GetParent(JEventLevel level) const {
    for (const auto& pair : mParents) {
        if (pair.first == level) return *(*(pair.second));
    }
    throw JException("Unable to find parent at level %s", 
                        toString(level).c_str());
}

void JEvent::SetParent(std::shared_ptr<JEvent>* parent) {
    JEventLevel level = parent->get()->GetLevel();
    for (const auto& pair : mParents) {
        if (pair.first == level) throw JException("Event already has a parent at level %s", 
                                                    toString(parent->get()->GetLevel()).c_str());
    }
    mParents.push_back({level, parent});
    parent->get()->mReferenceCount.fetch_add(1);
}

std::shared_ptr<JEvent>* JEvent::ReleaseParent(JEventLevel level) {
    if (mParents.size() == 0) {
        throw JException("ReleaseParent failed: child has no parents!");
    }
    auto pair = mParents.back();
    if (pair.first != level) {
        throw JException("JEvent::ReleaseParent called out of level order: Caller expected %s, but parent was actually %s", 
                toString(level).c_str(), toString(pair.first).c_str());
    }
    mParents.pop_back();
    auto remaining_refs = pair.second->get()->mReferenceCount.fetch_sub(1);
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

void JEvent::Release() {
    auto remaining_refs = mReferenceCount.fetch_sub(1);
    if (remaining_refs < 0) {
        throw JException("JEvent's own refcount has gone negative!");
    }
}

void JEvent::Reset() {
    mReferenceCount = 1;
}
