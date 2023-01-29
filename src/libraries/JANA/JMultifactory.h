
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JMULTIFACTORY_H
#define JANA2_JMULTIFACTORY_H

#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>

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
    // Alternatively, we could move all of the JMultiFactoryHelper functionality into JFactoryT directly
};


class JMultifactory {

    JFactorySet mHelpers; // This has ownership UNTIL JFactorySet::Add() takes it over

    std::once_flag m_is_initialized;
    std::once_flag m_is_finished;
    int32_t m_last_run_number = -1;
    // Remember where we are in the stream so that the correct sequence of callbacks get called.
    // However, don't worry about a Status variable. Every time Execute() gets called, so does Process()

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
    void DeclareOutput(std::string tag);

    template <typename T>
    void SetData(std::string tag, std::vector<T*> data);


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

};

template <typename T>
void JMultifactory::DeclareOutput(std::string tag) {
    JFactory* helper = new JMultifactoryHelper<T>(this);
    helper->SetTag(std::move(tag));
    mHelpers.Add(helper);
}

template <typename T>
void JMultifactory::SetData(std::string tag, std::vector<T *> data) {
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

template <typename T>
void JMultifactoryHelper<T>::Process(const std::shared_ptr<const JEvent> &event) {
    mMultiFactory->Execute(event);
}

// TODO: JFactoryGenerator lets you specify a instance-specific tag that overrides whatever tag
//       the class itself declared. This won't work with multifactories because one multifactory
//       has a number of different outputs, each with potentially different tags.
//       A. Maybe we could set an instance-level tag on the multifactory, and use that to mangle the factory tags?
//       B. Maybe we could pass in a vector of tags as a ctor arg, and have DeclareOutput and SetData/PublishOutput
//          reference the tags by index instead? [This one seems more promising]
//       This is just idle musing now, but maybe we could specify tags when calling DeclareInput() as well,
//          and then call event->Get<MyT>(InputTag(1))


#endif //JANA2_JMULTIFACTORY_H
