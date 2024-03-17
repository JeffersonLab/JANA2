// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Utils/JEventLevel.h>
#include <JANA/JEvent.h>
#include <mutex>

class JApplication;
class JEventUnfolder {

private:
    // Common to components... factor this out someday

    JEventLevel m_level = JEventLevel::Event;
    JApplication* m_application = nullptr;
    std::string m_plugin_name;
    std::string m_type_name;
    std::mutex m_mutex;
    int32_t m_last_run_number = -1;
    enum class Status { Uninitialized, Initialized, Finalized };
    Status m_status = Status::Uninitialized;
    booll m_enable_simplified_callbacks = false;

    // JEventUnfolder-specific
    //
    JEventLevel m_child_level;
    int m_per_timeslice_event_count = 0;
    bool m_call_preprocess_upstream = true;


public:
    // JEventUnfolder interface
    
    virtual ~JEventUnfolder() {};
 
    enum class Result { KeepGoing, Finished };

    virtual void Init() {};

    virtual void ChangeRun(const JEvent&) {};

    virtual void ChangeRun(uint64_t run_nr) {};

    virtual void Preprocess(const JEvent& /*parent*/) const {};
    
    virtual void Preprocess(uint64_t /*parent_nr*/) const {};

    virtual Result Unfold(const JEvent& parent, JEvent& child, int item_nr) {};

    virtual Result Unfold(uint64_t parent_nr, uint64_t child_nr, int item_nr) {};

    virtual void EndRun() {};

    virtual void Finish() {};


    // Configuration

    void SetParentLevel(JEventLevel level) { m_level = level; }

    void SetChildLevel(JEventLevel level) { m_child_level = level; }

    void SetCallPreprocessUpstream(bool call_upstream) { m_call_preprocess_upstream = call_upstream; }
    
    JEventLevel GetChildLevel() { return m_child_level; }


    // Component setters (set by user)
    
    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    void SetLevel(JEventLevel level) { m_level = level; }


    // Component getters
    
    JEventLevel GetLevel() { return m_level; }

    JApplication* GetApplication() const { return m_application; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }


 private:
    // Component setters (set by JANA)

    friend JComponentManager;

    friend JApplication;
    
    void SetApplication(JApplication* app) { m_application = app; }

    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

    // TODO: void SetInputCollectionNames()
    // TODO: void SetOutputCollectionNames()
    // TODO: void SetLogger()


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
            // TODO: We only want the mutex for checking the status
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status == Status::Initialized) {
                for (auto* input : m_inputs) {
                    input->GetCollection(parent);
                }
                if (m_enable_simplified_callbacks) {
                    Preprocess(parent.GetEventNumber());
                }
                else {
                    Preprocess(parent);
                }
            }
            else {
                throw JException("JEventUnfolder: Component needs to be initialized and not finalized before Unfold can be called");
            }
        }
        catch (JException& ex) {
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
                    if (m_enable_simplified_callbacks) {
                        Preprocess(parent.GetEventNumber());
                    }
                    else {
                        Preprocess(parent);
                    }
                }
                if (m_last_run_number != parent.GetRunNumber()) {
                    for (auto* resource : m_resources) {
                        resource->ChangeRun(parent);
                    }
                    if (m_enable_simplified_callbacks) {
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
                if (m_enable_simplified_callbacks) {
                    Result result = Unfold(parent.GetEventNumber(), child.GetEventNumber(), m_per_timeslice_event_count);
                }
                else {
                    Result result = Unfold(parent, child, m_per_timeslice_event_count);
                }
                for (auto* output : m_outputs) {
                    output->InsertCollection(child);
                }
                m_per_timeslice_event_count += 1;
                if (result == Result::Finished) {
                    m_per_timeslice_event_count = 0;
                }
                return result;
            }
            else {
                throw JException("Component needs to be initialized and not finalized before Unfold can be called");
            }
        }
        catch (JException& ex) {
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


