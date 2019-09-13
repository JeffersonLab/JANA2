//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

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

    JApplication(JParameterManager* params = nullptr);
    ~JApplication();


    // Loading plugins

    void AddPlugin(string plugin_name);
    void AddPluginPath(string path);


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
    void SetExitCode(int exit_code);
    int GetExitCode(void);


    // Performance/status monitoring

    bool IsQuitting(void) { return _quitting; }
    bool IsDrainingQueues(void) { return _draining_queues; }

    void SetTicker(bool ticker_on = true);
    void PrintStatus();
    void PrintFinalReport();
    uint64_t GetNThreads();
    uint64_t GetNeventsProcessed();
    float GetIntegratedRate();
    float GetInstantaneousRate();
    // TODO: Do we really want these?
    uint64_t GetNtasksCompleted(std::string name="") { return 0; }
    void GetInstantaneousRates(std::vector<double> &rates_by_queue) {}
    void GetIntegratedRates(std::map<std::string,double> &rates_by_thread) {}

    JComponentSummary GetComponentSummary();

    // Parameter config

    JParameterManager* GetJParameterManager() { return _params; }

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
    ///
    /// TODO: Consider making ServiceLocator be an argument to Factory::init(), etc?
    template <typename T>
    std::shared_ptr<T> GetService();

    /// Call this from InitPlugin.
    template <typename T>
    void ProvideService(std::shared_ptr<T> service);


private:

    JLogger _logger;
    JParameterManager* _params;
    JPluginLoader* _plugin_loader;
    JComponentManager* _component_manager;
    JProcessingController* _processing_controller;
    JServiceLocator _service_locator;

    bool _quitting = false;
    bool _draining_queues = false;
    bool _skip_join = false;
    bool _initialized = false;
    bool _ticker_on = true;
    bool _extended_report = true;
    int  _exit_code = 0;
    int  _desired_nthreads;


    std::chrono::time_point<std::chrono::high_resolution_clock> mRunStartTime;

    JResourcePool<JFactorySet> mFactorySetPool;
};



// Templates

/// A convenience method which delegates to JParameterManager
template<typename T>
T JApplication::GetParameterValue(std::string name) {
    return _params->GetParameterValue<T>(name);
}

/// A convenience method which delegates to JParameterManager
template<typename T>
JParameter* JApplication::SetParameterValue(std::string name, T val) {
    return _params->SetParameter(name, val);
}

template <typename T>
JParameter* JApplication::SetDefaultParameter(std::string name, T& val, std::string description) {
    return _params->SetDefaultParameter(name.c_str(), val, description);
}

template <typename T>
JParameter* JApplication::GetParameter(std::string name, T& result) {
    return _params->GetParameter(name, result);
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
std::shared_ptr<T> JApplication::GetService() {
    return _service_locator.get<T>();
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
void JApplication::ProvideService(std::shared_ptr<T> service) {
    _service_locator.provide(service);
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

