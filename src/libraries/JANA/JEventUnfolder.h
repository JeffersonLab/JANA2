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

    JEventLevel m_level;
    JApplication* m_application = nullptr;
    std::string m_plugin_name;
    std::string m_type_name;
    std::mutex m_mutex;
    int32_t m_last_run_number = -1;
    enum class Status { Uninitialized, Initialized, Finalized };
    Status m_status;

    // JEventUnfolder-specific
    //
    JEventLevel m_child_level;
    int m_per_timeslice_event_count = 0;
    bool m_call_preprocess_upstream = true;


public:
    // JEventUnfolder interface
 
    enum class Result { KeepGoing, Finished };

    virtual void Init() {};

    virtual void ChangeRun(const JEvent&) {};

    virtual void Preprocess(const JEvent& parent) const {};

    virtual Result Unfold(const JEvent& parent, JEvent& child, int item) = 0;

    virtual void EndRun() {};

    virtual void Finish() {};


    // Configuration

    void SetParentLevel(JEventLevel level) { m_level = level; }

    void SetChildLevel(JEventLevel level) { m_child_level = level; }

    void SetCallPreprocessUpstream(bool call_upstream) { m_call_preprocess_upstream = call_upstream; }


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
    
    void SetJApplication(JApplication* app) { m_application = app; }

    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };


 public:
    // Backend
    
    void DoInit() {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status == Status::Uninitialized) {
                Init();
                m_status = Status::Initialized;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventUnfolder::Init()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_type_name;
            throw ex;
        }
    }

    void DoPreprocess(const JEvent& parent, JEvent& child) {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status == Status::Initialized) {
                Preprocess(parent);
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
                    Preprocess(parent);
                }
                if (m_last_run_number != parent.GetRunNumber()) {
                    ChangeRun(parent);
                    m_last_run_number = parent.GetRunNumber();
                }
                Result result = Unfold(parent, child, m_per_timeslice_event_count);
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
            auto ex = JException("Unknown exception in JEventUnfolder::DoUnfold()");
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


