
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JMULTIFACTORY_H
#define JANA2_JMULTIFACTORY_H

#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>

#ifdef JANA2_HAVE_PODIO
#include <JANA/Podio/JPodioTypeHelpers.h>
#include "JANA/Podio/JFactoryPodioT.h"
#endif

class JMultifactory;

template <typename T>
class JMultifactoryHelper : public JFactoryT<T>{

    JMultifactory* mMultiFactory;

public:
    JMultifactoryHelper(JMultifactory* parent) : mMultiFactory(parent) {}
    virtual ~JMultifactoryHelper() = default;
    // This does NOT own mMultiFactory; the enclosing JFactorySet does

    void Process(const std::shared_ptr<const JEvent>&) override;
    // We really want to override Create, not Process!!!
    // It might make more sense to (1) put Create() back on JFactory, (2) make Create() virtual, (3) override Create()
    // Alternatively, we could move all the JMultiFactoryHelper functionality into JFactoryT directly

    JMultifactory* GetMultifactory() { return mMultiFactory; }
};


#ifdef JANA2_HAVE_PODIO
// TODO: This redundancy goes away if we merge JFactoryPodioT with JFactoryT
template <typename T>
class JMultifactoryHelperPodio : public JFactoryPodioT<T>{

    JMultifactory* mMultiFactory;

public:
    JMultifactoryHelperPodio(JMultifactory* parent) : mMultiFactory(parent) {}
    virtual ~JMultifactoryHelperPodio() = default;
    // This does NOT own mMultiFactory; the enclosing JFactorySet does

    void Process(const std::shared_ptr<const JEvent>&) override;
    // We really want to override Create, not Process!!!
    // It might make more sense to (1) put Create() back on JFactory, (2) make Create() virtual, (3) override Create()
    // Alternatively, we could move all of the JMultiFactoryHelper functionality into JFactoryT directly

    JMultifactory* GetMultifactory() { return mMultiFactory; }
};
#endif // JANA2_HAVE_PODIO


class JMultifactory {

    JFactorySet mHelpers; // This has ownership UNTIL JFactorySet::Add() takes it over

    std::once_flag m_is_initialized;
    std::once_flag m_is_finished;
    int32_t m_last_run_number = -1;
    // Remember where we are in the stream so that the correct sequence of callbacks get called.
    // However, don't worry about a Status variable. Every time Execute() gets called, so does Process().
    // The JMultifactoryHelpers will control calls to Execute().

    std::string mTagSuffix;  // In order to have multiple (differently configured) instances in the same factorySet
    std::string mPluginName; // So we can propagate this to the JMultifactoryHelpers, so we can have useful error messages
    std::string mFactoryName; // So we can propagate this to the JMultifactoryHelpers, so we can have useful error messages
    JApplication* mApp;

#ifdef JANA2_HAVE_PODIO
    bool mNeedPodio = false;      // Whether we need to retrieve the podio::Frame
    podio::Frame* mPodioFrame = nullptr;  // To provide the podio::Frame to SetPodioData, SetCollection
#endif

public:
    JMultifactory() = default;
    virtual ~JMultifactory() = default;

    // IMPLEMENTED BY USERS

    virtual void Init() {}
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    virtual void EndRun() {}
    virtual void Finish() {}
    // I'm tempted to factor out something like JEventCallback from JFactory, JMultifactory, and JEventProcessor.


    // CALLED BY USERS

    template <typename T>
    void DeclareOutput(std::string tag, bool owns_data=true);

    template <typename T>
    void SetData(std::string tag, std::vector<T*> data);

#ifdef JANA2_HAVE_PODIO

    template <typename T>
    void DeclarePodioOutput(std::string tag, bool owns_data=true);

    template <typename T>
    void SetCollection(std::string tag, typename JFactoryPodioT<T>::CollectionT&& collection);

    template <typename T>
    void SetCollection(std::string tag, std::unique_ptr<typename JFactoryPodioT<T>::CollectionT> collection);

#endif

    /// CALLED BY JANA

    void Execute(const std::shared_ptr<const JEvent>&);
    // Should this be execute or create? Who is tracking that this is called at most once per event?
    // Do we need something like JFactory::Status? Also, how do we ensure that CreationStatus is correct as well?

    void Release();
    // Release makes sure Finish() is called exactly once

    JFactorySet* GetHelpers();
    // This exposes the mHelpers JFactorySet, which contains a JFactoryT<T> for each declared output of the multifactory.
    // This is meant to be called from JFactorySet, which will take ownership of the helpers while leaving the pointers
    // in place. This method is only supposed to be called by JFactorySet::Add(JMultifactory).

    void SetApplication(JApplication* app) { mApp = app; }
    JApplication* GetApplication() { return mApp; }

    // These are set by JFactoryGeneratorT (just like JFactories) and get propagated to each of the JMultifactoryHelpers
    void SetTag(std::string tagSuffix) { mTagSuffix = std::move(tagSuffix); }
    void SetFactoryName(std::string factoryName) { mFactoryName = std::move(factoryName); }
    void SetPluginName(std::string pluginName) { mPluginName = std::move(pluginName); }
};



template <typename T>
void JMultifactory::DeclareOutput(std::string tag, bool owns_data) {
    JFactory* helper = new JMultifactoryHelper<T>(this);
    if (!owns_data) helper->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);
    tag += mTagSuffix;
    helper->SetPluginName(mPluginName);
    helper->SetFactoryName(mFactoryName);
    helper->SetTag(std::move(tag));
    mHelpers.Add(helper);
}

template <typename T>
void JMultifactory::SetData(std::string tag, std::vector<T*> data) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
#ifdef JANA2_HAVE_PODIO
    // This may or may not be a Podio factory. We find out if it is, and if so, set the frame before calling Set().
    auto* typed = dynamic_cast<JFactoryPodio*>(helper);
    if (typed != nullptr) {
        typed->SetFrame(mPodioFrame); // Needs to be called before helper->Set(), otherwise Set() excepts
    }
#endif
    helper->Set(data);
}


#ifdef JANA2_HAVE_PODIO

template <typename T>
void JMultifactory::DeclarePodioOutput(std::string tag, bool owns_data) {
    // TODO: Decouple tag name from collection name
    auto* helper = new JMultifactoryHelperPodio<T>(this);
    if (!owns_data) helper->SetSubsetCollection(true);

    tag += mTagSuffix;
    helper->SetTag(std::move(tag));
    helper->SetPluginName(mPluginName);
    helper->SetFactoryName(mFactoryName);
    mHelpers.Add(helper);
    mNeedPodio = true;
}

template <typename T>
void JMultifactory::SetCollection(std::string tag, typename JFactoryPodioT<T>::CollectionT&& collection) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
    auto* typed = dynamic_cast<JFactoryPodioT<T>*>(helper);
    if (typed == nullptr) {
        throw JException("JMultifactory: Helper needs to be a JFactoryPodioT (this shouldn't be reachable)");
    }

    typed->SetFrame(mPodioFrame);
    typed->SetCollection(std::move(collection));
}

template <typename T>
void JMultifactory::SetCollection(std::string tag, std::unique_ptr<typename JFactoryPodioT<T>::CollectionT> collection) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
    auto* typed = dynamic_cast<JFactoryPodioT<T>*>(helper);
    if (typed == nullptr) {
        throw JException("JMultifactory: Helper needs to be a JFactoryPodioT (this shouldn't be reachable)");
    }

    typed->SetFrame(mPodioFrame);
    typed->SetCollection(std::move(collection));
}

#endif // JANA2_HAVE_PODIO


template <typename T>
void JMultifactoryHelper<T>::Process(const std::shared_ptr<const JEvent> &event) {
    mMultiFactory->SetApplication(this->GetApplication());
    mMultiFactory->Execute(event);
}

#ifdef JANA2_HAVE_PODIO
template <typename T>
void JMultifactoryHelperPodio<T>::Process(const std::shared_ptr<const JEvent> &event) {
    mMultiFactory->SetApplication(this->GetApplication());
    mMultiFactory->Execute(event);
}
#endif // JANA2_HAVE_PODIO


#endif //JANA2_JMULTIFACTORY_H
