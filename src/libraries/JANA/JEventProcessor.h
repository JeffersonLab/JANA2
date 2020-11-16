
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JEventProcessor_h_
#define _JEventProcessor_h_

#include <JANA/JEvent.h>
#include <JANA/Utils/JTypeInfo.h>
#include <atomic>

class JApplication;


class JEventProcessor {
public:

    explicit JEventProcessor(JApplication* app = nullptr)
        : mApplication(app)
        , m_status(Status::Unopened)
        , m_event_count{0}
    {}

    virtual ~JEventProcessor() = default;

    enum class Status { Unopened, Opened, Finished };

    Status GetStatus() const { return m_status; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    JApplication* GetApplication() const { return mApplication; }

    bool AreEventsOrdered() const { return m_receive_events_in_order; }


    virtual void DoInitialize() {
        try {
            std::call_once(m_init_flag, &JEventProcessor::Init, this);
            m_status = Status::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // TODO: Improve this type signature
    virtual void DoMap(const std::shared_ptr<const JEvent>& e) {
        try {
            std::call_once(m_init_flag, &JEventProcessor::DoInitialize, this);
            auto run_number = e->GetRunNumber();
            if (m_last_run_number != run_number) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_last_run_number != -1) {
                    EndRun();
                }
	            m_last_run_number = run_number;
	            BeginRun(e);
            }
            Process(e);
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::DoMap()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // Reduce does nothing in the basic version because the current API tells
    // the user to lock a mutex in Process(), which takes care of it for us.
    virtual void DoReduce(const std::shared_ptr<const JEvent>& e) {}


    virtual void DoFinalize() {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Finished) {
                if (m_last_run_number != -1) {
                    EndRun();
                }
                Finish();
                m_status = Status::Finished;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Finish()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // TODO: Make these protected?
    /// JEventProcessor::Init, Process, and Finish are meant to be written by the user.
    /// Each JEventProcessor is intended to generate one distinct output,
    virtual void Init() {}

	virtual void BeginRun(const std::shared_ptr<const JEvent> &aEvent) {}

    virtual void Process(const std::shared_ptr<const JEvent>& aEvent) {
        throw JException("Not implemented yet!");
    }
	virtual void EndRun() {}

	virtual void Finish() {}


    // TODO: Can we please deprecate this in favor of GetTypeName?
    virtual std::string GetType() const {
        return m_type_name;
    }


protected:

    // The following are meant to be called by the user from the constructor in order to
    // configure their JEventProcessor instance.

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

private:
    Status m_status;
    std::string m_plugin_name;
    std::string m_type_name;
    std::string m_resource_name;
    std::once_flag m_init_flag;
    std::once_flag m_finish_flag;
    std::atomic_ullong m_event_count;
    size_t m_last_run_number = -1;
    std::mutex m_mutex;
    bool m_receive_events_in_order = false;

    /// This is called by JApplication::Add(JEventProcessor*). There
    /// should be no need to call it from anywhere else.
    void SetJApplication(JApplication* app) { mApplication = app; }

    friend JComponentManager;
    /// SetPluginName is called by ComponentManager and should not be exposed to the user.
    // TODO: Maybe we want JApplication to track the current plugin instead
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

};

#endif // _JEventProcessor_h_

