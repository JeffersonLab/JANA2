
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JMULTIFACTORY_H
#define JANA2_JMULTIFACTORY_H

#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>

#ifdef HAVE_PODIO
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
};


#ifdef HAVE_PODIO
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
};
#endif // HAVE_PODIO


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

#ifdef HAVE_PODIO
// We can dramatically simplify this code when we require JANA to use C++17 or higher.

    template <typename T, typename std::enable_if_t<is_podio_v<T>>* = nullptr>
    void DeclareOutput(std::string tag);

    template <typename T, typename std::enable_if_t<!is_podio_v<T>>* = nullptr>
    void DeclareOutput(std::string tag, bool owns_data=false);

    template <typename T, typename std::enable_if_t<is_podio_v<T>>* = nullptr>
    void SetData(std::string tag, std::vector<T*> data);

    template <typename T, typename std::enable_if_t<!is_podio_v<T>>* = nullptr>
    void SetData(std::string tag, std::vector<T*> data);

    template <typename T, typename std::enable_if_t<is_podio_v<T>>* = nullptr>
    void SetCollection(std::string tag, typename PodioTypeMap<T>::collection_t* collection);

#else

    template <typename T>
    void DeclareOutput(std::string tag, bool owns_data=true);

    template <typename T>
    void SetData(std::string tag, std::vector<T*> data);

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

    // These are set by JFactoryGeneratorT (just like JFactories) and get propagated to each of the JMultifactoryHelpers
    void SetTag(std::string tagSuffix) { mTagSuffix = std::move(tagSuffix); }
    void SetFactoryName(std::string factoryName) { mFactoryName = std::move(factoryName); }
    void SetPluginName(std::string pluginName) { mPluginName = std::move(pluginName); }
};



#ifdef HAVE_PODIO

template <typename T, typename std::enable_if_t<is_podio_v<T>>*>
void JMultifactory::DeclareOutput(std::string tag) {
    // TODO: Decouple tag name from collection name
    JFactory* helper = new JMultifactoryHelperPodio<T>(this);
    tag += mTagSuffix;
    helper->SetTag(std::move(tag));
    helper->SetPluginName(mPluginName);
    helper->SetFactoryName(mFactoryName);
    mHelpers.Add(helper);
}

template <typename T, typename std::enable_if_t<!is_podio_v<T>>*>
void JMultifactory::DeclareOutput(std::string tag, bool owns_data) {
    JFactory* helper = new JMultifactoryHelper<T>(this);
    if (!owns_data) helper->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);
    tag += mTagSuffix;
    helper->SetTag(std::move(tag));
    helper->SetPluginName(mPluginName);
    helper->SetFactoryName(mFactoryName);
    mHelpers.Add(helper);
}

template <typename T, typename std::enable_if_t<is_podio_v<T>>*>
void JMultifactory::SetData(std::string tag, std::vector<T*> data) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
    helper->Set(data);
}

template <typename T, typename std::enable_if_t<!is_podio_v<T>>*>
void JMultifactory::SetData(std::string tag, std::vector<T*> data) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
    helper->Set(data);
}

template <typename T, typename std::enable_if_t<is_podio_v<T>>*>
void JMultifactory::SetCollection(std::string tag, typename PodioTypeMap<T>::collection_t* collection) {
    JFactoryT<T>* helper = mHelpers.GetFactory<T>(tag);
    if (helper == nullptr) {
        throw JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
    }
    auto* typed = dynamic_cast<JFactoryPodioT<T>*>(helper);
    if (typed == nullptr) {
        throw JException("JMultifactory: Helper needs to be a JFactoryPodioT (this shouldn't be reachable)");
    }
    typed->SetCollection(collection);
}

#else // NOT HAVE_PODIO

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
    helper->Set(data);
    // Or we could just insert directly into the event
    // I can think of two reasons NOT to do that:
    // 1. Correctness and sanity checking, e.g. preventing users from clobbering other factories
    // 2. I eventually want to remove the ability to insert directly into the event from anywhere except EventSources
}

#endif // HAVE_PODIO


template <typename T>
void JMultifactoryHelper<T>::Process(const std::shared_ptr<const JEvent> &event) {
    mMultiFactory->Execute(event);
}

#ifdef HAVE_PODIO
template <typename T>
void JMultifactoryHelperPodio<T>::Process(const std::shared_ptr<const JEvent> &event) {
    mMultiFactory->Execute(event);
}
#endif // HAVE_PODIO


#endif //JANA2_JMULTIFACTORY_H
