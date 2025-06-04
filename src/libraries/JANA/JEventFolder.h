// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JComponent.h>
#include <JANA/Components/JHasInputs.h>
#include <JANA/Components/JHasOutputs.h>
#include <JANA/Components/JHasRunCallbacks.h>
#include <JANA/JEvent.h>

class JApplication;
class JEventFolder : public jana::components::JComponent, 
                     public jana::components::JHasRunCallbacks,
                     public jana::components::JHasInputs, 
                     public jana::components::JHasOutputs {

private:
    int32_t m_last_run_number = -1;
    JEventLevel m_child_level;
    bool m_call_preprocess_upstream = true;


public:
    
    JEventFolder() = default;
    virtual ~JEventFolder() {};
 
    virtual void Init() {};
    
    virtual void Preprocess(const JEvent& /*parent*/) const {};

    virtual void Fold(const JEvent& /*child*/, JEvent& /*parent*/, int /*item_nr*/) {
        throw JException("Not implemented yet!");
    };

    virtual void Finish() {};


    // Configuration

    void SetParentLevel(JEventLevel level) { m_level = level; }

    void SetChildLevel(JEventLevel level) { m_child_level = level; }

    void SetCallPreprocessUpstream(bool call_upstream) { m_call_preprocess_upstream = call_upstream; }
    
    JEventLevel GetChildLevel() { return m_child_level; }


 public:
    // Backend
    
    void DoInit() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Uninitialized) {
            throw JException("JEventFolder: Attempting to initialize twice or from an invalid state");
        }
        // TODO: Obtain overrides of collection names from param manager
        for (auto* parameter : m_parameters) {
            parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
        }
        for (auto* service : m_services) {
            service->Fetch(m_app);
        }
        CallWithJExceptionWrapper("JEventFolder::Init", [&](){Init();});
        m_status = Status::Initialized;
    }

    void DoPreprocess(const JEvent& child) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Initialized) {
                throw JException("JEventFolder: Component needs to be initialized and not finalized before Fold can be called");
            }
        }
        for (auto* input : m_inputs) {
            input->PrefetchCollection(child);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->PrefetchCollection(child);
        }
        if (m_callback_style != CallbackStyle::DeclarativeMode) {
            CallWithJExceptionWrapper("JEventFolder::Preprocess", [&](){
                Preprocess(child);
            });
        }
    }

    void DoFold(const JEvent& child, JEvent& parent) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Initialized) {
            throw JException("Component needs to be initialized and not finalized before Fold() can be called");
        }
        if (!m_call_preprocess_upstream) {
            CallWithJExceptionWrapper("JEventFolder::Preprocess", [&](){
                Preprocess(child);
            });
        }
        if (m_last_run_number != parent.GetRunNumber()) {
            for (auto* resource : m_resources) {
                resource->ChangeRun(parent.GetRunNumber(), m_app);
            }
            if (m_callback_style == CallbackStyle::DeclarativeMode) {
                CallWithJExceptionWrapper("JEventFolder::ChangeRun", [&](){
                    ChangeRun(parent.GetRunNumber());
                });
            }
            else {
                CallWithJExceptionWrapper("JEventFolder::ChangeRun", [&](){
                    ChangeRun(parent);
                });
            }
            m_last_run_number = parent.GetRunNumber();
        }
        for (auto* input : m_inputs) {
            input->GetCollection(child);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->GetCollection(child);
        }
        for (auto* output : m_outputs) {
            output->Reset();
        }
        auto child_number = child.GetEventIndex();
        CallWithJExceptionWrapper("JEventFolder::Fold", [&](){
            Fold(child, parent, child_number);
        });

        for (auto* output : m_outputs) {
            output->InsertCollection(parent);
        }
    }

    void DoFinish() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Finalized) {
            CallWithJExceptionWrapper("JEventFolder::Finish", [&](){
                Finish();
            });
            m_status = Status::Finalized;
        }
    }

    void Summarize(JComponentSummary& summary) const override {
        auto* us = new JComponentSummary::Component( 
            "Folder", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        for (const auto* input : m_inputs) {
            us->AddInput(new JComponentSummary::Collection("", input->GetDatabundleName(), input->type_name, input->level));
        }
        for (const auto* input : m_variadic_inputs) {
            size_t subinput_count = input->GetDatabundleNames().size();
            for (size_t i=0; i<subinput_count; ++i) {
                us->AddInput(new JComponentSummary::Collection("", input->GetDatabundleNames().at(i), input->type_name, input->level));
            }
        }
        for (const auto* output : m_outputs) {
            size_t suboutput_count = output->collection_names.size();
            for (size_t i=0; i<suboutput_count; ++i) {
                us->AddOutput(new JComponentSummary::Collection("", output->collection_names[i], output->type_name, GetLevel()));
            }
        }
        summary.Add(us);
    }

};


