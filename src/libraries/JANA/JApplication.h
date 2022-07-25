
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JApplication_h_
#define _JApplication_h_

#include <vector>
#include <string>
#include <iostream>

class JApplication;
class JEventProcessor;
class JEventSource;
class JEventSourceGenerator;
class JFactoryGenerator;
class JFactorySet;
class JComponentManager;
class JPluginLoader;
class JProcessingController;

extern JApplication* japp;

#include <JANA/Services/JServiceLocator.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JLoggingService.h>
#include <JANA/Status/JComponentSummary.h>
#include <JANA/Utils/JResourcePool.h>
#include <JANA/Status/JPerfSummary.h>


//////////////////////////////////////////////////////////////////////////////////////////////////
/// JANA application class (singleton).
///
/// The JApplication class serves as a central access point for getting to most things
/// in the JANA application. It owns the JThreadManager, JParameterManager, etc.
/// It is also responsible for making sure all of the plugins are attached and other
/// user specified configurations for the run are implemented before starting the processing
/// of the data. User code (e.g. plugins) will generally register things like event sources
/// and processors with the JApplication so they can be called up later at the appropriate time.
//////////////////////////////////////////////////////////////////////////////////////////////////
class JApplication {

public:

    /// These exit codes are what JANA uses internally. However they are fundamentally a suggestion --
    /// the user code is likely to use arbitrary exit codes.
    enum class ExitCode {Success=0, UnhandledException, Timeout, Segfault=139};

    explicit JApplication(JParameterManager* params = nullptr);
    ~JApplication();


    // Loading plugins

    void AddPlugin(std::string plugin_name);
    void AddPluginPath(std::string path);


    // Building a JProcessingTopology

    void Add(std::string event_source_name);
    void Add(JEventSourceGenerator* source_generator);
    void Add(JFactoryGenerator* factory_generator);
    void Add(JEventSource* event_source);
    void Add(JEventProcessor* processor);


    // Controlling processing

    void Initialize(void);
    void Run(bool wait_until_finished = true);
    void Scale(int nthreads);
    void Stop(bool wait_until_idle = false);
    void Resume() {};  // TODO: Do we need this?
    void Quit(bool skip_join = false);
    void SetExitCode(int exitCode);
    int GetExitCode();


    // Performance/status monitoring

    bool IsInitialized(void){return m_initialized;}
    bool IsQuitting(void) { return m_quitting; }
    bool IsDrainingQueues(void) { return m_draining_queues; }

    void SetTicker(bool ticker_on = true);
    void SetTimeoutEnabled(bool enabled = true);
    bool IsTimeoutEnabled();
    void PrintStatus();
    void PrintFinalReport();
    uint64_t GetNThreads();
    uint64_t GetNEventsProcessed();
    float GetIntegratedRate();
    float GetInstantaneousRate();

    JComponentSummary GetComponentSummary();

    // Parameter config

    JParameterManager* GetJParameterManager() { return m_params.get(); }

    template<typename T>
    T GetParameterValue(std::string name);

    template <typename T>
    JParameter* GetParameter(std::string name, T& val);

    template<typename T>
    JParameter* SetParameterValue(std::string name, T val);

    template <typename T>
    JParameter* SetDefaultParameter(std::string name, T& val, std::string description="");

    // Locating services

    /// Use this in EventSources, Factories, or EventProcessors. Do not call this
    /// from InitPlugin(), as not all JServices may have been loaded yet.
    /// When initializing a Service, use acquire_services() instead.
    template <typename T>
    std::shared_ptr<T> GetService();

    /// Call this from InitPlugin.
    template <typename T>
    void ProvideService(std::shared_ptr<T> service);


private:

    JLogger m_logger;
    JServiceLocator m_service_locator;

    std::shared_ptr<JParameterManager> m_params;
    std::shared_ptr<JPluginLoader> m_plugin_loader;
    std::shared_ptr<JComponentManager> m_component_manager;
    std::shared_ptr<JProcessingController> m_processing_controller;

    bool m_quitting = false;
    bool m_draining_queues = false;
    bool m_skip_join = false;
    bool m_initialized = false;
    bool m_ticker_on = true;
    bool m_timeout_on = true;
    bool m_extended_report = false;
    int  m_exit_code = (int) ExitCode::Success;
    int  m_desired_nthreads;

    std::mutex m_status_mutex;
    std::chrono::milliseconds m_ticker_interval {500};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_last_measurement;
    std::unique_ptr<const JPerfSummary> m_perf_summary;

    void update_status();
};



/// A convenience method which delegates to JParameterManager
template<typename T>
T JApplication::GetParameterValue(std::string name) {
    return m_params->GetParameterValue<T>(name);
}

/// A convenience method which delegates to JParameterManager
template<typename T>
JParameter* JApplication::SetParameterValue(std::string name, T val) {
    return m_params->SetParameter(name, val);
}

template <typename T>
JParameter* JApplication::SetDefaultParameter(std::string name, T& val, std::string description) {
    return m_params->SetDefaultParameter(name.c_str(), val, description);
}

template <typename T>
JParameter* JApplication::GetParameter(std::string name, T& result) {
    return m_params->GetParameter(name, result);
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
std::shared_ptr<T> JApplication::GetService() {
    return m_service_locator.get<T>();
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
void JApplication::ProvideService(std::shared_ptr<T> service) {
    m_service_locator.provide(service);
}



// This routine is used to bootstrap plugins. It is done outside
// of the JApplication class to ensure it sees the global variables
// that the rest of the plugin's InitPlugin routine sees.
inline void InitJANAPlugin(JApplication* app) {
    // Make sure global pointers are pointing to the
    // same ones being used by executable
    japp = app;
}

#endif // _JApplication_h_

