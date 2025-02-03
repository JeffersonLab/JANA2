// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JComponent.h>
#include <JANA/Components/JHasInputs.h>
#include <JANA/Components/JHasOutputs.h>
#include <JANA/Components/JHasRunCallbacks.h>
#include <JANA/JEvent.h>

class JApplication;
class JEventUnfolder : public jana::components::JComponent, 
                       public jana::components::JHasRunCallbacks,
                       public jana::components::JHasInputs, 
                       public jana::components::JHasOutputs {

private:
    int32_t m_last_run_number = -1;
    bool m_enable_simplified_callbacks = false;
    JEventLevel m_child_level;
    int m_child_number = 0;
    bool m_call_preprocess_upstream = true;


public:
    // JEventUnfolder interface
    
    virtual ~JEventUnfolder() {};
 
    enum class Result { NextChildNextParent, NextChildKeepParent, KeepChildNextParent };

    virtual void Init() {};
    
    virtual void Preprocess(const JEvent& /*parent*/) const {};

    virtual Result Unfold(const JEvent& /*parent*/, JEvent& /*child*/, int /*item_nr*/) {
        throw JException("Not implemented yet!");
    };

    virtual Result Unfold(uint64_t /*parent_nr*/, uint64_t /*child_nr*/, int /*item_nr*/) {
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
            throw JException("JEventUnfolder: Attempting to initialize twice or from an invalid state");
        }
        // TODO: Obtain overrides of collection names from param manager
        for (auto* parameter : m_parameters) {
            parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
        }
        for (auto* service : m_services) {
            service->Fetch(m_app);
        }
        CallWithJExceptionWrapper("JEventUnfolder::Init", [&](){Init();});
        m_status = Status::Initialized;
    }

    void DoPreprocess(const JEvent& parent) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Initialized) {
                throw JException("JEventUnfolder: Component needs to be initialized and not finalized before Unfold can be called");
                // TODO: Consider calling Initialize(with_lock=false) like we do elsewhere
            }
        }
        for (auto* input : m_inputs) {
            input->PrefetchCollection(parent);
        }
        if (m_callback_style != CallbackStyle::DeclarativeMode) {
            CallWithJExceptionWrapper("JEventUnfolder::Preprocess", [&](){
                Preprocess(parent);
            });
        }
    }

    Result DoUnfold(const JEvent& parent, JEvent& child) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status == Status::Initialized) {
            if (!m_call_preprocess_upstream) {
                if (!m_enable_simplified_callbacks) {
                    CallWithJExceptionWrapper("JEventUnfolder::Preprocess", [&](){
                        Preprocess(parent);
                    });
                }
            }
            if (m_last_run_number != parent.GetRunNumber()) {
                for (auto* resource : m_resources) {
                    resource->ChangeRun(parent.GetRunNumber(), m_app);
                }
                if (m_callback_style == CallbackStyle::DeclarativeMode) {
                    CallWithJExceptionWrapper("JEventUnfolder::ChangeRun", [&](){
                        ChangeRun(parent.GetRunNumber());
                    });
                }
                else {
                    CallWithJExceptionWrapper("JEventUnfolder::ChangeRun", [&](){
                        ChangeRun(parent);
                    });
                }
                m_last_run_number = parent.GetRunNumber();
            }
            for (auto* input : m_inputs) {
                input->GetCollection(parent);
                // TODO: This requires that all inputs come from the parent.
                //       However, eventually we will want to support inputs 
                //       that come from the child.
            }
            for (auto* output : m_outputs) {
                output->Reset();
            }
            Result result;
            child.SetEventIndex(m_child_number);
            if (m_enable_simplified_callbacks) {
                CallWithJExceptionWrapper("JEventUnfolder::Unfold", [&](){
                    result = Unfold(parent.GetEventNumber(), child.GetEventNumber(), m_child_number);
                });
            }
            else {
                CallWithJExceptionWrapper("JEventUnfolder::Unfold", [&](){
                    result = Unfold(parent, child, m_child_number);
                });
            }
            for (auto* output : m_outputs) {
                output->InsertCollection(child);
            }
            m_child_number += 1;
            if (result == Result::NextChildNextParent || result == Result::KeepChildNextParent) {
                m_child_number = 0;
            }
            return result;
        }
        else {
            throw JException("Component needs to be initialized and not finalized before Unfold can be called");
        }
    }

    void DoFinish() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Finalized) {
            CallWithJExceptionWrapper("JEventUnfolder::Finish", [&](){
                Finish();
            });
            m_status = Status::Finalized;
        }
    }

    void Summarize(JComponentSummary& summary) const override {
        auto* us = new JComponentSummary::Component( 
            "Unfolder", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        for (const auto* input : m_inputs) {
            size_t subinput_count = input->names.size();
            for (size_t i=0; i<subinput_count; ++i) {
                us->AddInput(new JComponentSummary::Collection("", input->names[i], input->type_name, input->levels[i]));
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


