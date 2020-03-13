//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JABSTRACTEVENTPROCESSOR_H
#define JANA2_JABSTRACTEVENTPROCESSOR_H

#include <memory>
#include <mutex>
#include <string>

class JEvent;
class JApplication;

class JAbstractEventProcessor {
public:

    virtual ~JAbstractEventProcessor() = default;

    virtual void DoInitialize() = 0;

    virtual void DoMap(const std::shared_ptr<const JEvent>& e) = 0;

    virtual void DoReduce(const std::shared_ptr<const JEvent>& e) {}

    virtual void DoFinalize() = 0;


    enum class Status { Unopened, Opened, Finished };

    Status GetStatus() const { return m_status; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    JApplication* GetApplication() const { return mApplication; }

    bool AreEventsOrdered() const { return m_receive_events_in_order; }

protected:

    // The following are meant to be called by the user from the constructor in order to
    // configure their JAbstractEventProcessor instance.

    /// SetTypeName is intended as a replacement to GetType(), which should be less confusing for the
    /// user. It should be called from the constructor. For convenience, we provide a
    /// NAME_OF_THIS macro so that the user doesn't have to type the class name as a string, which may
    /// get out of sync if automatic refactoring tools are used.

    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    /// Resource name lets the user tell the parallelization engine to synchronize different EventProcessors
    /// which write to the same shared resource; e.g. if you have two EventProcessors
    /// which both write to a ROOT tree, they should both set the resource name 'ROOT'. On the flip side,
    /// if you have two EventProcessors which write to different resources, e.g. ROOT and a CSV file, and
    /// you set different resource names, the parallelization engine will know that it is safe to pipeline
    /// these two processors. If you don't set a resource name at all, the parallelization engine will
    /// assume that you are manually synchronizing access via your own mutex, which will be safe if and only
    /// if you use your locks correctly, and also may result in a performance penalty.

    void SetResourceName(std::string resource_name) { m_resource_name = std::move(resource_name); }

    /// SetEventsOrdered allows the user to tell the parallelization engine that it needs to see
    /// the event stream ordered by increasing event IDs. (Note that this requires all EventSources
    /// emit event IDs which are consecutive.) Ordering by event ID makes for cleaner output, but comes
    /// with a performance penalty, so it is best if this is enabled during debugging, and disabled otherwise.

    void SetEventsOrdered(bool receive_events_in_order) { m_receive_events_in_order = receive_events_in_order; }

    // TODO: Stop getting mApplication this way, use GetApplication() instead, or pass directly to Init()
    JApplication* mApplication = nullptr;

protected:
    Status m_status = Status::Unopened;
    std::string m_plugin_name;
    std::string m_type_name;

private:
    std::string m_resource_name;
    std::once_flag m_init_flag;
    std::once_flag m_finish_flag;
    std::atomic_ullong m_event_count {0};
    bool m_receive_events_in_order = false;

    /// This is called by JApplication::Add(JAbstractEventProcessor*). There
    /// should be no need to call it from anywhere else.
    void SetJApplication(JApplication* app) { mApplication = app; }

    friend class JComponentManager;
    /// SetPlugin is called by ComponentManager and should not be exposed to the user.
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

};



#endif //JANA2_JABSTRACTEVENTPROCESSOR_H
