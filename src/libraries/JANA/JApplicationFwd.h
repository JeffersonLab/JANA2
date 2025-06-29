
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>

class JEventProcessor;
class JEventSource;
class JEventSourceGenerator;
class JFactoryGenerator;
class JFactorySet;
class JComponentManager;
class JPluginLoader;
class JExecutionEngine;
class JEventUnfolder;
class JServiceLocator;
class JParameter;
class JParameterManager;
class JApplication;
extern JApplication* japp;

#include <JANA/Components/JComponentSummary.h>
#include <JANA/JLogger.h>


//////////////////////////////////////////////////////////////////////////////////////////////////
/// JANA application class
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
    explicit JApplication(JLogger::Level verbosity);
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
    void Add(JEventUnfolder* unfolder);


    // Controlling processing

    void Initialize(void);
    void Run(bool wait_until_stopped=true, bool finish=true);
    void Scale(int nthreads);
    void Stop(bool wait_until_stopped=false, bool finish=true);
    void Inspect();
    void Quit(bool skip_join = false);
    void SetExitCode(int exitCode);
    int GetExitCode();

    // Performance/status monitoring
    void PrintStatus();
    bool IsInitialized(void){return m_initialized;}
    bool IsQuitting(void) { return m_quitting; }
    bool IsDrainingQueues();

    void SetTicker(bool ticker_on = true);
    bool IsTickerEnabled();
    void SetTimeoutEnabled(bool enabled = true);
    bool IsTimeoutEnabled();
    uint64_t GetNThreads();
    uint64_t GetNEventsProcessed();
    float GetIntegratedRate();
    float GetInstantaneousRate();

    const JComponentSummary& GetComponentSummary();

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

    template <typename T>
    T RegisterParameter(std::string name, const T default_val, std::string description="");

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

    std::unique_ptr<JServiceLocator> m_service_locator;

    std::shared_ptr<JParameterManager> m_params;
    std::shared_ptr<JPluginLoader> m_plugin_loader;
    std::shared_ptr<JComponentManager> m_component_manager;
    std::shared_ptr<JExecutionEngine> m_execution_engine;

    bool m_inspect = false;
    bool m_inspecting = false;
    bool m_quitting = false;
    bool m_skip_join = false;
    std::atomic_bool m_initialized {false};
    std::atomic_bool m_services_available {false};
    int  m_exit_code = (int) ExitCode::Success;
    int  m_desired_nthreads;
    std::atomic_int m_sigint_count {0};

    // For instantaneous rate calculations
    std::mutex m_inst_rate_mutex;
    std::chrono::steady_clock::time_point m_last_measurement_time = std::chrono::steady_clock::now();
    size_t m_last_event_count = 0;
};


// This routine is used to bootstrap plugins. It is done outside
// of the JApplication class to ensure it sees the global variables
// that the rest of the plugin's InitPlugin routine sees.
inline void InitJANAPlugin(JApplication* app) {
    // Make sure global pointers are pointing to the
    // same ones being used by executable
    japp = app;
}


