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
    JEventFolder() {
        m_type_name = "JEventFolder";
    }
    virtual ~JEventFolder() {};
 
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

    void DoPreprocess(const JEvent& child) {
        {
            if (!m_is_initialized) {
                throw JException("JEventFolder: Component needs to be initialized and not finalized before Fold can be called");
            }
        }
        for (auto* input : m_inputs) {
            input->TriggerFactoryCreate(child);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->TriggerFactoryCreate(child);
        }
        CallWithJExceptionWrapper("JEventFolder::Preprocess", [&](){
            Preprocess(child);
        });
    }

    void DoFold(const JEvent& child, JEvent& parent) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_is_initialized) {
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
            CallWithJExceptionWrapper("JEventFolder::ChangeRun", [&](){
                ChangeRun(parent);
            });
            m_last_run_number = parent.GetRunNumber();
        }
        for (auto* input : m_inputs) {
            input->Populate(child);
        }
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->Populate(child);
        }
        auto child_number = child.GetEventIndex();
        CallWithJExceptionWrapper("JEventFolder::Fold", [&](){
            Fold(child, parent, child_number);
        });

        for (auto* output : GetOutputs()) {
            output->EulerianStore(*parent.GetFactorySet());
        }
        for (auto* output : GetVariadicOutputs()) {
            output->EulerianStore(*parent.GetFactorySet());
        }
    }

    void DoFinish() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_is_finalized) {
            CallWithJExceptionWrapper("JEventFolder::Finish", [&](){
                Finish();
            });
            m_is_finalized = true;
        }
    }

    void Summarize(JComponentSummary& summary) const override {
        auto* us = new JComponentSummary::Component( 
            "Folder", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        SummarizeInputs(*us);
        SummarizeOutputs(*us);
        summary.Add(us);
    }

};


