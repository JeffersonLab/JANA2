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

#define jout std::cout
#define jerr std::cerr

class JApplication;
class JEventProcessor;
class JEventSource;
class JEventSourceGenerator;
class JFactoryGenerator;
class JEventSourceManager;
class JThreadManager;
class JFactorySet;

class JPluginLoader;
class JProcessingController;
class JTopologyBuilder;
struct JProcessingTopology;

extern JApplication* japp;

#include <JANA/JLogger.h>
#include <JANA/JParameterManager.h>

// TODO: Move these down one level
#include <JANA/JResourcePool.h>
#include <JANA/JResourcePoolSimple.h>
#include <JANA/JTask.h>


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

    // Parameter config

    JParameterManager* GetJParameterManager() { return _params; }

    template<typename T>
    T GetParameterValue(std::string name);

    template<typename T>
    JParameter* SetParameterValue(std::string name, T val);


    // Doesn't belong here

    void UpdateResourceLimits(void);
    void GetJEventProcessors(std::vector<JEventProcessor*>& aProcessors);
    void GetJFactoryGenerators(std::vector<JFactoryGenerator*>& factory_generators);
    JThreadManager* GetJThreadManager(void) const;
    JEventSourceManager* GetJEventSourceManager(void) const;
    std::shared_ptr<JTask<void>> GetVoidTask(void);
    JFactorySet* GetFactorySet(void);
    void Recycle(JFactorySet* aFactorySet);
    string Val2StringWithPrefix(float val);


private:

    JLogger _logger;
    JParameterManager* _params;
    JPluginLoader* _plugin_loader;
    JTopologyBuilder* _topology_builder;
    JProcessingTopology* _topology;
    JProcessingController* _processing_controller;

    bool _quitting = false;
    bool _draining_queues = false;
    bool _skip_join = false;
    bool _initialized = false;
    bool _ticker_on = true;
    bool _extended_report = true;
    int  _exit_code = 0;
    int  _desired_nthreads;


    std::chrono::time_point<std::chrono::high_resolution_clock> mRunStartTime;

    // TODO: Get rid of these
    JResourcePoolSimple<JFactorySet> mFactorySetPool;
    JResourcePool<JTask<void>> mVoidTaskPool;
    JThreadManager* _threadManager = nullptr; // Extract this from LegacyProcessingController

};



// Templates
template<typename T>
T JApplication::GetParameterValue(std::string name) {
    /// This is a convenience function that just calls the method
    /// of the same name in JParameterManager.
    return GetJParameterManager()->GetParameterValue<T>(name);
}

template<typename T>
JParameter* JApplication::SetParameterValue(std::string name, T val) {
    /// This is a convenience function that just calls the SetParameter
    /// of JParameterManager.
    return GetJParameterManager()->SetParameter(name, val);
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

