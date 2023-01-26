
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JMULTIFACTORY_H
#define JANA2_JMULTIFACTORY_H

#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JFactoryGenerator.h>

class JMultifactory;

template <typename T>
class JMultifactoryHelper : public JFactoryT<T>{

    std::shared_ptr<JMultifactory> mMultiFactory;

public:
    void Process(const std::shared_ptr<const JEvent>&) override;
    // We really want to override Create, not Process!!!
    // It might make more sense to (1) put Create() back on JFactory, (2) make Create() virtual, (3) override Create()
    // Alternatively, we could move all of the JMultiFactoryHelper functionality into JFactoryT directly
};


class JMultifactory {

    JFactorySet mHelpers;
    // TODO: PROBLEM WITH OWNERSHIP OF HELPERS
    // Does mHelpers have ownership of the JMultifactoryHelpers, or does the JEvent's JFactorySet?
    // Assume that the JMultifactoryGenerator transfers ownership over.
    // Meanwhile the ownership of the Multifactory itself is shared by the different helpers

    std::once_flag m_is_initialized;
    std::once_flag m_is_finished;
    int32_t m_last_run_number = -1;
    // Remember where we are in the stream so that the correct sequence of callbacks get called.
    // However, don't worry about a Status variable. Every time Execute() gets called, so does Process()

public:
    virtual void Init() {}
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    virtual void EndRun() {}
    virtual void Finish() {}
    // I'm tempted to factor out something like JEventCallback from JFactory, JMultifactory, and JEventProcessor.

    /// CALLED BY JANA

    void Execute(const std::shared_ptr<const JEvent>&);
    // Should this be execute or create? Who is tracking that this is called at most once per event?
    // Do we need something like JFactory::Status? Also, how do we ensure that CreationStatus is correct as well?

    void Release();
    // Release makes sure Finish() is called exactly once

    JFactorySet* GetHelpers();
    // This exposes the JFactorySet. Note that the caller can remove JFactories from the set, and/or claim ownership!

    /// CALLED BY USERS

    template <typename T>
    void DeclareOutput(std::string tag);

    template <typename T>
    void SetData(std::string tag, std::vector<T*> data);
};

template <typename T>
void JMultifactory::DeclareOutput(std::string tag) {
    JFactory* helper = new JMultifactoryHelper<T>;
    helper->SetTag(tag);
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

template <typename T>
class JMultifactoryGeneratorT : public JFactoryGenerator {
    void GenerateFactories(JFactorySet *factory_set) override;
};


// TODO: JFactoryGenerator lets you specify a instance-specific tag that overrides whatever tag
//       the class itself declared. This won't work with multifactories because one multifactory
//       has a number of different outputs, each with potentially different tags.
//       A. Maybe we could set an instance-level tag on the multifactory, and use that to mangle the factory tags?
//       B. Maybe we could pass in a vector of tags as a ctor arg, and have DeclareOutput and SetData/PublishOutput
//          reference the tags by index instead? [This one seems more promising]
//       This is just idle musing now, but maybe we could specify tags when calling DeclareInput() as well,
//          and then call event->Get<MyT>(InputTag(1))
template <typename T>
void JMultifactoryGeneratorT<T>::GenerateFactories(JFactorySet *factory_set) {
    JMultifactory *multifactory = new T;
    // Two options:
    // 1. Keep helpers inside mHelpers.
    //    - Requires making JFactorySet ownership semantics much more ambiguous
    //    -
    // 2. Remember (typeid, tag), but hand over mHelpers
    //    - This keeps JFactorySet::Merge semantics intact (as well as ownership semantics)
    //    - This means that we deepen our dependence on JEvent::Insert in a way I don't like
    //    - This is sketchy because we are throwing away the string-typeid fallback

    // Outputs should have all been declared in the multifactory constructor, so it is ready to access the helper factories

    for (JFactory* fac : multifactory->GetHelpers()->GetAllFactories()) {
        fac->SetFactoryName(JTypeInfo::demangle<T>());
        fac->SetPluginName(GetPluginName());
        factory_set->Add(fac);
    }

    // TODO: Finish me!
    //multifactory->GetHelpers()->SetOwnership(false);


}



#endif //JANA2_JMULTIFACTORY_H
