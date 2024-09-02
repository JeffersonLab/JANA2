
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JHasInputs.h>
#include <JANA/Omni/JHasRunCallbacks.h>
#include <JANA/JEvent.h>

class JApplication;


class JEventProcessor : public jana::omni::JComponent, 
                        public jana::omni::JHasRunCallbacks, 
                        public jana::omni::JHasInputs {
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
        for (auto* parameter : m_parameters) {
            parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
        }
        for (auto* service : m_services) {
            service->Init(m_app);
        }
        CallWithJExceptionWrapper("JEventProcessor::Init", [&](){ Init(); });
        m_status = Status::Initialized;
    }


    virtual void DoMap(const std::shared_ptr<const JEvent>& e) {

        if (m_callback_style == CallbackStyle::LegacyMode) {
            throw JException("Called DoMap() on a legacy-mode JEventProcessor");
        }
        for (auto* input : m_inputs) {
            input->PrefetchCollection(*e);
        }
        if (m_callback_style == CallbackStyle::ExpertMode) {
            ProcessParallel(*e);
        }
        else {
            ProcessParallel(e->GetRunNumber(), e->GetEventNumber(), e->GetEventIndex());
        }
    }


    virtual void DoTap(const std::shared_ptr<const JEvent>& e) {

        if (m_callback_style == CallbackStyle::LegacyMode) {
            throw JException("Called DoReduce() on a legacy-mode JEventProcessor");
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        // In principle DoReduce() is being called by one thread at a time, but we hold a lock anyway 
        // so that this runs correctly even if that isn't happening. This lock shouldn't experience
        // any contention.

        if (m_status == Status::Uninitialized) {
            DoInitialize();
        }
        else if (m_status == Status::Finalized) {
            throw JException("JEventProcessor: Attempted to call DoMap() after Finalize()");
        }
        for (auto* input : m_inputs) {
            // This collection should have already been computed during DoMap()
            // We do this before ChangeRun() just in case we will need to pull data out of
            // a begin-of-run event.
            input->GetCollection(*e);
        }
        auto run_number = e->GetRunNumber();
        if (m_last_run_number != run_number) {
            if (m_last_run_number != -1) {
                CallWithJExceptionWrapper("JEventProcessor::EndRun", [&](){ EndRun(); });
            }
            for (auto* resource : m_resources) {
                resource->ChangeRun(e->GetRunNumber(), m_app);
            }
            m_last_run_number = run_number;
            CallWithJExceptionWrapper("JEventProcessor::BeginRun", [&](){ BeginRun(e); });
        }
        if (m_callback_style == CallbackStyle::DeclarativeMode) {
            CallWithJExceptionWrapper("JEventProcessor::Process", [&](){ 
                Process(e->GetRunNumber(), e->GetEventNumber(), e->GetEventIndex());
            });
        }
        else if (m_callback_style == CallbackStyle::ExpertMode) {
            CallWithJExceptionWrapper("JEventProcessor::Process", [&](){ Process(*e); });
        }
        m_event_count += 1;
    }


    virtual void DoLegacyProcess(const std::shared_ptr<const JEvent>& event) {

        // DoLegacyProcess doesn't hold any locks, as it requires the user to hold a lock for it.
        // Because of this, 
        if (m_callback_style != CallbackStyle::LegacyMode) {
            throw JException("Called DoLegacyProcess() on a non-legacy-mode JEventProcessor");
        }

        auto run_number = event->GetRunNumber();

        if (m_status == Status::Uninitialized) {
            DoInitialize();
        }
        else if (m_status == Status::Finalized) {
            throw JException("JEventProcessor: Attempted to call DoMap() after Finalize()");
        }
        if (m_last_run_number != run_number) {
            if (m_last_run_number != -1) {
                CallWithJExceptionWrapper("JEventProcessor::EndRun", [&](){ EndRun(); });
            }
            for (auto* resource : m_resources) {
                resource->ChangeRun(event->GetRunNumber(), m_app);
            }
            m_last_run_number = run_number;
            CallWithJExceptionWrapper("JEventProcessor::BeginRun", [&](){ BeginRun(event); });
        }
        CallWithJExceptionWrapper("JEventProcessor::Process", [&](){ Process(event); });
        m_event_count += 1;
    }


    virtual void DoFinalize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Finalized) {
            if (m_last_run_number != -1) {
                CallWithJExceptionWrapper("JEventProcessor::EndRun", [&](){ EndRun(); });
            }
            CallWithJExceptionWrapper("JEventProcessor::Finish", [&](){ Finish(); });
            m_status = Status::Finalized;
        }
    }


    void Summarize(JComponentSummary& summary) const override {
        auto* result = new JComponentSummary::Component(
            "Processor", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        for (const auto* input : m_inputs) {
            size_t subinput_count = input->names.size();
            for (size_t i=0; i<subinput_count; ++i) {
                result->AddInput(new JComponentSummary::Collection("", input->names[i], input->type_name, input->levels[i]));
            }
        }
        summary.Add(result);
    }


    virtual void Init() {}


    // LegacyMode-specific callbacks

    virtual void Process(const std::shared_ptr<const JEvent>& /*event*/) {
        throw JException("Not implemented yet!");
    }
    
    // ExpertMode-specific callbacks

    virtual void ProcessParallel(const JEvent& /*event*/) {
    }

    virtual void Process(const JEvent& /*event*/) {
        throw JException("Not implemented yet!");
    }

    // DeclarativeMode-specific callbacks

    virtual void ProcessParallel(int64_t /*run_nr*/, uint64_t /*event_nr*/, uint64_t /*event_idx*/) {
        throw JException("Not implemented yet!");
    }

    virtual void Process(int64_t /*run_nr*/, uint64_t /*event_nr*/, uint64_t /*event_idx*/) {
        throw JException("Not implemented yet!");
    }


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
    bool m_receive_events_in_order = false;

};


