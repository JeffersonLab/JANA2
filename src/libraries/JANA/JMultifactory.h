
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JHasRunCallbacks.h>

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


class JMultifactory : public jana::omni::JComponent,
                      public jana::omni::JHasRunCallbacks {

    JFactorySet mHelpers; // This has ownership UNTIL JFactorySet::Add() takes it over

    // Remember where we are in the stream so that the correct sequence of callbacks get called.
    // However, don't worry about a Status variable. Every time Execute() gets called, so does Process().
    // The JMultifactoryHelpers will control calls to Execute().

    std::string mTag;         // JMultifactories each get their own name
                              // This can be used for parameter and collection name prefixing, though at a higher level
    std::string mFactoryName; // So we can propagate this to the JMultifactoryHelpers, so we can have useful error messages

#ifdef JANA2_HAVE_PODIO
    bool mNeedPodio = false;      // Whether we need to retrieve the podio::Frame
    podio::Frame* mPodioFrame = nullptr;  // To provide the podio::Frame to SetPodioData, SetCollection
#endif

public:
    JMultifactory() = default;
    virtual ~JMultifactory() = default;

    // IMPLEMENTED BY USERS

    virtual void Init() {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    void EndRun() override {}
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
    
    void DoInit();

    void Execute(const std::shared_ptr<const JEvent>&);
    // Should this be execute or create? Who is tracking that this is called at most once per event?
    // Do we need something like JFactory::Status? Also, how do we ensure that CreationStatus is correct as well?

    void Release();
    // Release makes sure Finish() is called exactly once

    JFactorySet* GetHelpers();
    // This exposes the mHelpers JFactorySet, which contains a JFactoryT<T> for each declared output of the multifactory.
    // This is meant to be called from JFactorySet, which will take ownership of the helpers while leaving the pointers
    // in place. This method is only supposed to be called by JFactorySet::Add(JMultifactory).

    // These are set by JFactoryGeneratorT (just like JFactories) and get propagated to each of the JMultifactoryHelpers
    void SetTag(std::string tag) { mTag = std::move(tag); }
    void SetFactoryName(std::string factoryName) { mFactoryName = std::move(factoryName); }
    
    void Summarize(JComponentSummary& summary) override;
};



template <typename T>
void JMultifactory::DeclareOutput(std::string tag, bool owns_data) {
    auto* helper = new JMultifactoryHelper<T>(this);
    if (!owns_data) helper->SetNotOwnerFlag(false);
    helper->SetPluginName(m_plugin_name);
    helper->SetFactoryName(mFactoryName);
    helper->SetTag(std::move(tag));
    helper->SetLevel(GetLevel());
    mHelpers.SetLevel(GetLevel());
    mHelpers.Add(helper);
}

template <typename T>
void JMultifactory::SetData(std::string tag, std::vector<T*> data) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        auto ex = JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
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

    helper->SetTag(std::move(tag));
    helper->SetPluginName(m_plugin_name);
    helper->SetFactoryName(mFactoryName);
    helper->SetLevel(GetLevel());
    mHelpers.SetLevel(GetLevel());
    mHelpers.Add(helper);
    mNeedPodio = true;
}

template <typename T>
void JMultifactory::SetCollection(std::string tag, typename JFactoryPodioT<T>::CollectionT&& collection) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        auto ex = JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
        ex.function_name = "JMultifactory::SetCollection";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    auto* typed = dynamic_cast<JFactoryPodioT<T>*>(helper);
    if (typed == nullptr) {
        auto ex = JException("JMultifactory: Helper needs to be a JFactoryPodioT (this shouldn't be reachable)");
        ex.function_name = "JMultifactory::SetCollection";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }

    typed->SetFrame(mPodioFrame);
    typed->SetCollection(std::move(collection));
}

template <typename T>
void JMultifactory::SetCollection(std::string tag, std::unique_ptr<typename JFactoryPodioT<T>::CollectionT> collection) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        auto ex = JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
        ex.function_name = "JMultifactory::SetCollection";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    auto* typed = dynamic_cast<JFactoryPodioT<T>*>(helper);
    if (typed == nullptr) {
        auto ex = JException("JMultifactory: Helper needs to be a JFactoryPodioT (this shouldn't be reachable)");
        ex.function_name = "JMultifactory::SetCollection";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
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


