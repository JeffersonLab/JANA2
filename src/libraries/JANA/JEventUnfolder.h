// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JHasInputs.h>
#include <JANA/Omni/JHasOutputs.h>
#include <JANA/Omni/JHasRunCallbacks.h>
#include <JANA/JEvent.h>

class JApplication;
class JEventUnfolder : public jana::omni::JComponent, 
                       public jana::omni::JHasRunCallbacks,
                       public jana::omni::JHasInputs, 
                       public jana::omni::JHasOutputs {

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
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            // TODO: Obtain overrides of collection names from param manager
            
            for (auto* parameter : m_parameters) {
                parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
            }
            for (auto* service : m_services) {
                service->Init(m_app);
            }
            if (m_status == Status::Uninitialized) {
                Init();
                m_status = Status::Initialized;
            }
            else {
                throw JException("JEventUnfolder: Attempting to initialize twice or from an invalid state");
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("JEventUnfolder: Exception in Init()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }

    void DoPreprocess(const JEvent& parent) {
        try {
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
                Preprocess(parent);
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            if (ex.component_name.empty()) {
                ex.component_name = m_type_name;
            }
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException("Exception in JEventUnfolder::DoPreprocess(): %s", e.what());
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventUnfolder::DoPreprocess()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }

    Result DoUnfold(const JEvent& parent, JEvent& child) {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status == Status::Initialized) {
                if (!m_call_preprocess_upstream) {
                    if (!m_enable_simplified_callbacks) {
                        Preprocess(parent);
                    }
                }
                if (m_last_run_number != parent.GetRunNumber()) {
                    for (auto* resource : m_resources) {
                        resource->ChangeRun(parent.GetRunNumber(), m_app);
                    }
                    if (m_callback_style == CallbackStyle::DeclarativeMode) {
                        ChangeRun(parent.GetRunNumber());
                    }
                    else {
                        ChangeRun(parent);
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
                    result = Unfold(parent.GetEventNumber(), child.GetEventNumber(), m_child_number);
                }
                else {
                    result = Unfold(parent, child, m_child_number);
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
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            if (ex.component_name.empty()) {
                ex.component_name = m_type_name;
            }
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException("Exception in JEventUnfolder::DoUnfold(): %s", e.what());
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Exception in JEventUnfolder::DoUnfold()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }

    void DoFinish() {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Finalized) {
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
            auto ex = JException("Unknown exception in JEventUnfolder::Finish()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }
};


