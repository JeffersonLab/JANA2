
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

class JApplication;
class JParameterManager;

#include <JANA/Utils/JEventLevel.h>
#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Components/JComponentSummary.h>

#include <vector>
#include <mutex>

namespace jana::components {


struct JComponent {
    enum class Status { Uninitialized, Initialized, Opened, Closed, Finalized };
    enum class CallbackStyle { LegacyMode, ExpertMode, DeclarativeMode };

    struct ParameterBase;
    struct ServiceBase;

protected:
    std::vector<ParameterBase*> m_parameters;
    std::vector<ServiceBase*> m_services;
    
    JEventLevel m_level = JEventLevel::PhysicsEvent;
    CallbackStyle m_callback_style = CallbackStyle::LegacyMode;
    std::string m_prefix;
    std::string m_plugin_name;
    std::string m_logger_name;
    std::string m_type_name;
    Status m_status = Status::Uninitialized;
    mutable std::mutex m_mutex;
    JApplication* m_app = nullptr;
    JLogger m_logger;

public:
    JComponent() = default;
    virtual ~JComponent() = default;

    // ---------------------
    // Meant to be called by users, or alternatively from a Generator
    // ---------------------
    void SetLevel(JEventLevel level) { m_level = level; }

    void SetCallbackStyle(CallbackStyle style) { m_callback_style = style; }

    void SetPrefix(std::string prefix) {
        m_prefix = prefix;
    }
    /// For convenience, we provide a NAME_OF_THIS macro so that the user doesn't have to store the type name as a string, 
    /// because that could get out of sync if automatic refactoring tools are used.
    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    JApplication* GetApplication() const { 
        if (m_app == nullptr) {
            throw JException("JApplication pointer hasn't been provided yet! Hint: Component configuration should happen inside Init(), not in the constructor.");
        }
        return m_app; 
    }

    JLogger& GetLogger() { return m_logger; }


    // ---------------------
    // Meant to be called by JANA
    // ---------------------

    void Wire(JApplication* app);

    std::string GetPrefix() const { return m_prefix.empty() ? m_type_name : m_prefix; }

    JEventLevel GetLevel() const { return m_level; }

    std::string GetLoggerName() const {
        if (!m_logger_name.empty()) return m_logger_name;
        if (!m_prefix.empty()) return m_prefix;
        if (!m_type_name.empty()) return m_type_name;
        return "";
    }

    std::string GetPluginName() const { return m_plugin_name; }

    void SetLoggerName(std::string logger_name) { m_logger_name = std::move(logger_name); }

    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

    std::string GetTypeName() const { return m_type_name; }

    virtual void Summarize(JComponentSummary&) const {};

    CallbackStyle GetCallbackStyle() const { return m_callback_style; }

    Status GetStatus() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_status; 
    }

    void SetApplication(JApplication* app) { 
        if (app == nullptr) {
            throw JException("Attempting to set a null JApplication pointer!");
        }
        m_app = app; 
    }

    void SetLogger(JLogger logger) { m_logger = logger; }

    template <typename F> 
    inline void CallWithJExceptionWrapper(std::string func_name, F func);

    void DoInit();

    // `Init` is where the user requests parameters and services. If the user requests all parameters and services here,
    // JANA can report them back to the user without having to open the resource and run the topology.
    virtual void Init() {};

    // ---------------------
    // "Registered member" helpers
    // ---------------------

    struct ParameterBase {
        std::string m_name; // Does NOT include component prefix, which is provided by owner
        std::string m_description;
        bool m_is_shared = false;

        void SetName(std::string name) { m_name = name; }
        void SetDescription(std::string description) { m_description = description; }
        void SetShared(bool is_shared) { m_is_shared = is_shared; }

        const std::string& GetName() { return m_name; }
        const std::string& GetDescription() { return m_description; }
        bool IsShared() { return m_is_shared; }

        virtual void Init(JParameterManager& parman, const std::string& prefix) = 0;
        virtual void Wire(const std::map<std::string, std::string>& isolated, const std::map<std::string, std::string>& shared) = 0;
    };

    template <typename T> 
    class ParameterRef;

    template <typename T> 
    class Parameter;

    struct ServiceBase {
        virtual void Fetch(JApplication* app) = 0;
    };

    template <typename T> 
    class Service;

    void RegisterParameter(ParameterBase* parameter) {
        m_parameters.push_back(parameter);
    }

    void RegisterService(ServiceBase* service) {
        m_services.push_back(service);
    }

    const std::vector<ParameterBase*> GetAllParameters() const {
        return this->m_parameters;
    }

};


} // namespace jana::components


