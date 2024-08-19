
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

class JApplication;
class JParameterManager;

#include <JANA/Utils/JEventLevel.h>
#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Status/JComponentSummary.h>

#include <vector>
#include <mutex>

namespace jana {
namespace omni {


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
    std::string m_type_name;
    Status m_status = Status::Uninitialized;
    mutable std::mutex m_mutex;
    JApplication* m_app = nullptr;
    JLogger m_logger;

public:
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
    std::string GetPrefix() const { return m_prefix.empty() ? m_type_name : m_prefix; }

    JEventLevel GetLevel() const { return m_level; }

    std::string GetLoggerName() const { return m_prefix.empty() ? m_type_name : m_prefix; }

    std::string GetPluginName() const { return m_plugin_name; }

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

    // ---------------------
    // "Registered member" helpers
    // ---------------------

    struct ParameterBase {
        std::string m_name;
        std::string m_description;
        virtual void Configure(JParameterManager& parman, const std::string& prefix) = 0;
        virtual void Configure(std::map<std::string, std::string> fields) = 0;
    };

    template <typename T> 
    class ParameterRef;

    template <typename T> 
    class Parameter;

    struct ServiceBase {
        virtual void Init(JApplication* app) = 0;
    };

    template <typename T> 
    class Service;

    void RegisterParameter(ParameterBase* parameter) {
        m_parameters.push_back(parameter);
    }

    void RegisterService(ServiceBase* service) {
        m_services.push_back(service);
    }

    void ConfigureAllParameters(std::map<std::string, std::string> fields) {
        for (auto* parameter : this->m_parameters) {
            parameter->Configure(fields);
        }
    }


    // 
};



} // namespace omni
} // namespace jana


