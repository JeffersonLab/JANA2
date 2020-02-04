//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JABSTRACTEVENTSOURCE_H
#define JANA2_JABSTRACTEVENTSOURCE_H

#include <memory>
#include <JANA/JEvent.h>

class JAbstractEventSource {
public:

    virtual ~JAbstractEventSource() = default;

    /// SourceStatus describes the current state of the EventSource
    enum class SourceStatus { Unopened, Opened, Finished };

    /// ReturnStatus describes what happened the last time a DoNext() was attempted.
    /// If DoNext() reaches an error state, it should throw a JException instead.
    /// Note that TryAgain and Finish both denote failure states, i.e. the JEvent was _not_
    /// successfully populated with real data.
    enum class ReturnStatus { Success, TryAgain, Finished };

    /// Initialize the EventSource while preserving all invariants
    virtual void DoInitialize() = 0;

    /// Attempt to emit one event while preserving all invariants
    virtual ReturnStatus DoNext(std::shared_ptr<JEvent> event) = 0;



    SourceStatus GetStatus() const { return m_status; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    JApplication* GetApplication() const { return m_application; }

    virtual std::string GetType() const { return m_type_name; }

    std::string GetName() const { return m_resource_name; }

    // TODO: Move this onto V1 and V2 impls instead
    //This should create default factories for all types available in the event source
    JFactoryGenerator* GetFactoryGenerator() const { return m_factory_generator; }

    // Meant to be called by user
    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    // Meant to be called by user
    void SetFactoryGenerator(JFactoryGenerator* generator) { m_factory_generator = generator; }

    // Meant to be called by JANA
    void SetApplication(JApplication* app) { m_application = app; }

    // Meant to be called by JANA
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

    // Meant to be called by JANA
    void SetRange(uint64_t nskip, uint64_t nevents) {
        m_nskip = nskip;
        m_nevents = nevents;
    };

protected:
    std::string m_resource_name;
    JApplication* m_application = nullptr;
    JFactoryGenerator* m_factory_generator = nullptr;
    SourceStatus m_status;
    std::atomic_ullong m_event_count {0};
    uint64_t m_nskip = 0;
    uint64_t m_nevents = 0;

    std::string m_plugin_name;
    std::string m_type_name;
};

#endif //JANA2_JABSTRACTEVENTSOURCE_H
