
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JComponent.h>
#include <JANA/Components/JHasInputs.h>
#include <JANA/Components/JHasRunCallbacks.h>
#include <JANA/JEvent.h>
#include <mutex>

class JApplication;


class JEventProcessor : public jana::components::JComponent, 
                        public jana::components::JHasRunCallbacks, 
                        public jana::components::JHasInputs {
public:

    JEventProcessor() {
        m_type_name = "JEventProcessor";
    }
    virtual ~JEventProcessor() = default;

    [[deprecated]]
    explicit JEventProcessor(JApplication* app) {
        m_app = app;
    }


    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    bool IsOrderingEnabled() const { return m_enable_ordering; }

    void EnableOrdering(bool enable=true) { m_enable_ordering = enable; }


    virtual void DoMap(const JEvent& event) {

        if (m_callback_style == CallbackStyle::LegacyMode) {
            throw JException("Called DoMap() on a legacy-mode JEventProcessor");
        }
        for (auto* input : m_inputs) {
            input->TriggerFactoryCreate(event);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->TriggerFactoryCreate(event);
        }
        if (m_callback_style == CallbackStyle::ExpertMode) {
            ProcessParallel(event);
        }
        else {
            ProcessParallel(event.GetRunNumber(), event.GetEventNumber(), event.GetEventIndex());
        }
    }


    virtual void DoTap(const JEvent& event) {

        if (m_callback_style == CallbackStyle::LegacyMode) {
            throw JException("Called DoReduce() on a legacy-mode JEventProcessor");
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        // In principle DoReduce() is being called by one thread at a time, but we hold a lock anyway 
        // so that this runs correctly even if that isn't happening. This lock shouldn't experience
        // any contention.

        if (!m_is_initialized) {
            throw JException("JEventProcessor: Attempted to call DoTap() before Initialize()");
        }
        else if (m_is_finalized) {
            throw JException("JEventProcessor: Attempted to call DoMap() after Finalize()");
        }
        for (auto* input : m_inputs) {
            // This collection should have already been computed during DoMap()
            // We do this before ChangeRun() just in case we will need to pull data out of
            // a begin-of-run event.
            input->Populate(event);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->Populate(event);
        }
        auto run_number = event.GetRunNumber();
        if (m_last_run_number != run_number) {
            for (auto* resource : m_resources) {
                resource->ChangeRun(event.GetRunNumber(), m_app);
            }
            m_last_run_number = run_number;
            CallWithJExceptionWrapper("JEventProcessor::ChangeRun", [&](){ ChangeRun(event); });
        }
        if (m_callback_style == CallbackStyle::DeclarativeMode) {
            CallWithJExceptionWrapper("JEventProcessor::ProcessSequential", [&](){ 
                ProcessSequential(event.GetRunNumber(), event.GetEventNumber(), event.GetEventIndex());
            });
        }
        else if (m_callback_style == CallbackStyle::ExpertMode) {
            CallWithJExceptionWrapper("JEventProcessor::ProcessSequential", [&](){ ProcessSequential(event); });
        }
        m_event_count += 1;
    }


    virtual void DoLegacyProcess(const std::shared_ptr<const JEvent>& event) {

        // DoLegacyProcess holds a lock to make sure that {Begin,Change,End}Run() are always called before Process(). 
        // Note that in LegacyMode, Process() requires the user to manage a _separate_ lock for its critical section.
        // This arrangement means that {Begin,Change,End}Run() will definitely be called at least once before `Process`, but there
        // may be races when there are multiple run numbers present in the stream. This isn't a problem in practice for now, 
        // but future work should use ExpertMode or DeclarativeMode for this reason (but also for the usability improvements!)

        if (m_callback_style != CallbackStyle::LegacyMode) {
            throw JException("Called DoLegacyProcess() on a non-legacy-mode JEventProcessor");
        }

        auto run_number = event->GetRunNumber();

        {
            // Protect the call to BeginRun(), etc, to prevent some threads from running Process() before BeginRun().
            std::lock_guard<std::mutex> lock(m_mutex);

            if (!m_is_initialized) {
                throw JException("JEventProcessor: Attempted to call DoLegacyProcess() before Initialize()");
            }
            else if (m_is_finalized) {
                throw JException("JEventProcessor: Attempted to call DoLegacyProcess() after Finalize()");
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
        }
        CallWithJExceptionWrapper("JEventProcessor::Process", [&](){ Process(event); });
        m_event_count += 1;
    }


    virtual void DoFinalize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_is_finalized) {
            if (m_last_run_number != -1) {
                CallWithJExceptionWrapper("JEventProcessor::EndRun", [&](){ EndRun(); });
            }
            CallWithJExceptionWrapper("JEventProcessor::Finish", [&](){ Finish(); });
            m_is_finalized = true;
        }
    }


    void Summarize(JComponentSummary& summary) const override {
        auto* result = new JComponentSummary::Component(
            "Processor", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        for (const auto* input : m_inputs) {
            result->AddInput(new JComponentSummary::Collection("", input->GetDatabundleName(), input->GetTypeName(), input->GetLevel()));
        }
        for (const auto* input : m_variadic_inputs) {
            size_t subinput_count = input->GetRequestedDatabundleNames().size();
            for (size_t i=0; i<subinput_count; ++i) {
                result->AddInput(new JComponentSummary::Collection("", input->GetRequestedDatabundleNames()[i], input->GetTypeName(), input->GetLevel()));
            }
        }
        summary.Add(result);
    }


    // LegacyMode-specific callbacks

    virtual void Process(const std::shared_ptr<const JEvent>& /*event*/) {
    }

    // ExpertMode-specific callbacks

    virtual void ProcessParallel(const JEvent& /*event*/) {
    }

    virtual void ProcessSequential(const JEvent& /*event*/) {
    }

    // DeclarativeMode-specific callbacks

    virtual void ProcessParallel(int64_t /*run_nr*/, uint64_t /*event_nr*/, uint64_t /*event_idx*/) {
    }

    virtual void ProcessSequential(int64_t /*run_nr*/, uint64_t /*event_nr*/, uint64_t /*event_idx*/) {
    }


    virtual void Finish() {}


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


private:
    bool m_enable_ordering = false;
    std::string m_resource_name;
    std::atomic_ullong m_event_count {0};

};


