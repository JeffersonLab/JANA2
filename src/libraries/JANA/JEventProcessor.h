
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JEventProcessor_h_
#define _JEventProcessor_h_

#include <JANA/JEvent.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Omni/JComponent.h>
#include <atomic>

class JApplication;


class JEventProcessor : public jana::omni::JComponent {
public:

    JEventProcessor() = default;
    virtual ~JEventProcessor() = default;

    // TODO: Deprecate
    explicit JEventProcessor(JApplication* app) {
        m_app = app;
    }


    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    bool AreEventsOrdered() const { return m_receive_events_in_order; }


    virtual void DoInitialize() {
        try {
            for (auto* parameter : m_parameters) {
                parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
            }
            for (auto* service : m_services) {
                service->Init(m_app);
            }
            Init();
            m_status = Status::Initialized;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }


    virtual void DoMap(const std::shared_ptr<const JEvent>& e) {
        try {
            auto run_number = e->GetRunNumber();
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (m_status == Status::Uninitialized) {
                    DoInitialize();
                }
                else if (m_status == Status::Finalized) {
                    throw JException("JEventProcessor: Attempted to call DoMap() after Finalize()");
                }

                if (m_last_run_number != run_number) {
                    if (m_last_run_number != -1) {
                        EndRun();
                    }
                    for (auto* resource : m_resources) {
                        resource->ChangeRun(*(e.get()));
                    }
                    m_last_run_number = run_number;
                    BeginRun(e);
                }
            }
            Process(e);
            m_event_count += 1;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException(e.what());
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::DoMap()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }


    // Reduce does nothing in the basic version because the current API tells
    // the user to lock a mutex in Process(), which takes care of it for us.
    virtual void DoReduce(const std::shared_ptr<const JEvent>&) {}


    virtual void DoFinalize() {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Finalized) {
                if (m_last_run_number != -1) {
                    EndRun();
                }
                Finish();
                m_status = Status::Finalized;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Finish()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }


    virtual void Init() {}

    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}

    virtual void Process(const std::shared_ptr<const JEvent>& /*event*/) {
        throw JException("Not implemented yet!");
    }
    virtual void EndRun() {}

    virtual void Finish() {}


    // TODO: Deprecate
    virtual std::string GetType() const {
        return m_type_name;
    }

protected:

    // The following are meant to be called by the user from the constructor in order to
    // configure their JEventProcessor instance.

    /// Resource name lets the user tell the parallelization engine to synchronize different EventProcessors
    /// which write to the same shared resource; e.g. if you have two EventProcessors
    /// which both write to a ROOT tree, they should both set the resource name 'ROOT'. On the flip side,
    /// if you have two EventProcessors which write to different resources, e.g. ROOT and a CSV file, and
    /// you set different resource names, the parallelization engine will know that it is safe to pipeline
    /// these two processors. If you don't set a resource name at all, the parallelization engine will
    /// assume that you are manually synchronizing access via your own mutex, which will be safe if and only
    /// if you use your locks correctly, and also may result in a performance penalty.

    // void SetResourceName(std::string resource_name) { m_resource_name = std::move(resource_name); }

    /// SetEventsOrdered allows the user to tell the parallelization engine that it needs to see
    /// the event stream ordered by increasing event IDs. (Note that this requires all EventSources
    /// emit event IDs which are consecutive.) Ordering by event ID makes for cleaner output, but comes
    /// with a performance penalty, so it is best if this is enabled during debugging, and disabled otherwise.

    // void SetEventsOrdered(bool receive_events_in_order) { m_receive_events_in_order = receive_events_in_order; }


private:
    std::string m_resource_name;
    std::atomic_ullong m_event_count {0};
    int32_t m_last_run_number = -1;
    bool m_receive_events_in_order = false;

};

#endif // _JEventProcessor_h_

