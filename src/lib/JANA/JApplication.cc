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

#include "JApplication.h"
#include "JLegacyProcessingController.h"
#include "JArrowProcessingController.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JParameterManager.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JPluginLoader.h>
#include <JANA/JTopologyBuilder.h>
#include <JANA/JProcessingController.h>

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    _params = (params == nullptr) ? new JParameterManager : params;

    _logger = JLoggingService::logger("JApplication");
    _plugin_loader = new JPluginLoader(this,_params);
    _topology_builder = new JTopologyBuilder();
    _topology = nullptr;
    _processing_controller = nullptr;
}


JApplication::~JApplication() {
    if (_processing_controller != nullptr) {
        delete _processing_controller;
    }
    delete _params;
    delete _plugin_loader;
    delete _topology_builder;
    if (_topology != nullptr) {
        delete _topology;
    }
}


// Loading plugins

void JApplication::AddPlugin(std::string plugin_name) {
    _plugin_loader->add_plugin(plugin_name);
}

void JApplication::AddPluginPath(std::string path) {
    _plugin_loader->add_plugin_path(path);
}


// Building a ProcessingTopology

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param source_generator pointer to source generator to add. Ownership is passed to JApplication
    _topology_builder->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    _topology_builder->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    _topology_builder->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    _topology_builder->add(event_source_name);
}


// Controlling processing

void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not
    /// need to call this directly.

    // Only run this once
    if (_initialized) return;
    _initialized = true;

    // Attach all plugins
    _plugin_loader->attach_plugins(_topology_builder);

    // Set task pool size
    int task_pool_size = 200;
    int task_pool_debuglevel = 0;
    _params->SetDefaultParameter("JANA:TASK_POOL_SIZE", task_pool_size, "Task pool size");
    _params->SetDefaultParameter("JANA:TASK_POOL_DEBUGLEVEL", task_pool_debuglevel, "Task pool debug level");
    mVoidTaskPool.Set_ControlParams(task_pool_size, task_pool_debuglevel);
    // TODO: Move mVoidTaskPool into JThreadManager

    // Set desired nthreads
    _desired_nthreads = JCpuInfo::GetNumCpus();
    _params->SetDefaultParameter("NTHREADS", _desired_nthreads,
                                 "The total number of worker threads");


    _topology = _topology_builder->build_topology();

    bool legacy_mode = true;
    _params->SetDefaultParameter("JANA:LEGACY_MODE", legacy_mode, "");
    if (legacy_mode) {
        auto * pc = new JLegacyProcessingController(this, _topology);
        _threadManager = pc->get_threadmanager();
        _processing_controller = pc;
        _extended_report = false;
    }
    else {
        _processing_controller = new JArrowProcessingController(_topology);
        _extended_report = true;
    }

    _params->SetDefaultParameter("JANA:EXTENDED_REPORT", _extended_report);
    _processing_controller->initialize();
}

void JApplication::Run() {

    Initialize();
    if(_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    _params->PrintParameters(false);

    jout << "Start processing ..." << std::endl;
    mRunStartTime = std::chrono::high_resolution_clock::now();
    _processing_controller->run(_desired_nthreads);

    // Monitor status of all threads
    while( !_quitting ){

        // If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
        // This flag is used by the integrated rate calculator
        // The JThreadManager is in charge of telling all the threads to end
        if(!_draining_queues)
            _draining_queues = _topology->event_source_manager.AreAllFilesClosed();


        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (!_topology->is_active() || _processing_controller->is_stopped()) {
            std::cout << "All threads have ended.\n";
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for( std::chrono::milliseconds(500) );

        // Print status
        if( _ticker_on ) PrintStatus();
    }

    // Join all threads
    if (!_skip_join) {
        jout << "Merging threads ..." << std::endl;
        _processing_controller->wait_until_stopped();
    }

    jout << "Event processing ended. " << std::endl;
    // Report Final numbers
    PrintFinalReport();
}

void JApplication::Scale(int nthreads) {
    _processing_controller->scale(nthreads);
}

void JApplication::Stop(bool wait_until_idle) {
    _processing_controller->request_stop();
    if (wait_until_idle) {
        _processing_controller->wait_until_stopped();
    }
}

void JApplication::Quit(bool skip_join) {
    _skip_join = skip_join;
    _quitting = true;
    if (_processing_controller != nullptr) {
        Stop(skip_join);
    }
}

void JApplication::SetExitCode(int exit_code) {
    /// Set a value of the exit code in that can be later retrieved
    /// using GetExitCode. This is so the executable can return
    /// a meaningful error code if processing is stopped prematurely,
    /// but the program is able to stop gracefully without a hard
    /// exit. See also GetExitCode.

    _exit_code = exit_code;
}

int JApplication::GetExitCode() {
	/// Returns the currently set exit code. This can be used by
	/// JProcessor/JFactory classes to communicate an appropriate
	/// exit code that a jana program can return upon exit. The
	/// value can be set via the SetExitCode method.

	return _exit_code;
}


// Performance/status monitoring

void JApplication::SetTicker(bool ticker_on) {
    _ticker_on = ticker_on;
}

void JApplication::PrintStatus(void) {
    if (_extended_report) {
        _processing_controller->print_report();
    }
    else {
        std::stringstream ss;
        ss << "  " << GetNeventsProcessed() << " events processed  " << Val2StringWithPrefix( GetInstantaneousRate() ) << "Hz (" << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz avg)             ";
        jout << ss.str() << "\n";
        jout.flush();
    }
}

void JApplication::PrintFinalReport() {

    if (_extended_report) {
        _processing_controller->print_final_report();
    }
    else {
        jout << std::endl;
        auto nevents = GetNeventsProcessed();
        jout << "Number of threads: " << GetNThreads() << std::endl;
        jout << "Total events processed: " << nevents << " (~ " << Val2StringWithPrefix( nevents ) << "evt)" << std::endl;
        jout << "Integrated Rate: " << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz" << std::endl;
        jout << std::endl;
    }
    if (_extended_report) {
        //_plugin_loader->print_report();
        //_topology->print_report();
        //jout << std::endl;
        //jout << "               Num. plugins: " << _plugins.size() <<std::endl;
        //jout << "          Num. plugin paths: " << _plugin_paths.size() <<std::endl;
        //jout << "    Num. factory generators: " << _factoryGenerators.size() <<std::endl;
        //jout << "      Num. event processors: " << mNumProcessorsAdded <<std::endl;
        //jout << "          Num. factory sets: " << mFactorySetPool.Get_PoolSize() << " (max. " << mFactorySetPool.Get_MaxPoolSize() << ")" << std::endl;
    }
}

uint64_t JApplication::GetNThreads() {
    return _processing_controller->get_nthreads();
}

uint64_t JApplication::GetNeventsProcessed() {
    return _processing_controller->get_nevents_processed();
    //return _eventSourceManager->GetNumEventsProcessed();
}

float JApplication::GetIntegratedRate() {
    /// Returns the total integrated rate so far in Hz since
    /// Run was called.
    static float last_R = 0.0;

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> delta_t = now - mRunStartTime;
    float delta_t_seconds = delta_t.count();
    float delta_N = (float)GetNeventsProcessed();

    if( delta_t_seconds >= 0.5) {
        last_R = delta_N / delta_t_seconds;
    }

    return last_R;
}

float JApplication::GetInstantaneousRate()
{
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t N = GetNeventsProcessed();
    static auto last_t = now;
    static uint64_t last_N = N;

    std::chrono::duration<float> delta_t = now - last_t;
    float delta_t_seconds = delta_t.count();
    float delta_N = (float)(GetNeventsProcessed() - last_N);

    static float last_R = 0.0;
    if( delta_t_seconds >= 0.5) {
        last_R = delta_N / delta_t_seconds;
        last_t = now;
        last_N = N;
    }

    return last_R;
}



// Things that don't belong here

std::shared_ptr<JTask<void>> JApplication::GetVoidTask() {
    return mVoidTaskPool.Get_SharedResource();
}

JFactorySet* JApplication::GetFactorySet() {
    return mFactorySetPool.Get_Resource(_topology->factory_generators);
}

void JApplication::Recycle(JFactorySet* aFactorySet) {
    return mFactorySetPool.Recycle(aFactorySet);
}

JEventSourceManager* JApplication::GetJEventSourceManager() const {
    return &_topology->event_source_manager;
}

void JApplication::GetJEventProcessors(std::vector<JEventProcessor*>& aProcessors) {
    aProcessors = _topology->event_processors;
}

void JApplication::GetJFactoryGenerators(std::vector<JFactoryGenerator*> &factory_generators) {
    factory_generators = _topology->factory_generators;
}

std::string JApplication::Val2StringWithPrefix(float val)
{
    /// Return the value as a string with the appropriate latin unit prefix
    /// appended.
    /// Values returned are: "G", "M", "k", "", "u", and "m" for
    /// values of "val" that are: >1.5E9, >1.5E6, >1.5E3, <1.0E-7, <1.0E-4, 1.0E-1
    /// respectively.
    const char *units = "";
    if(val>1.5E9){
        val/=1.0E9;
        units = "G";
    }else 	if(val>1.5E6){
        val/=1.0E6;
        units = "M";
    }else if(val>1.5E3){
        val/=1.0E3;
        units = "k";
    }else if(val<1.0E-7){
        units = "";
    }else if(val<1.0E-4){
        val/=1.0E6;
        units = "u";
    }else if(val<1.0E-1){
        val/=1.0E3;
        units = "m";
    }

    char str[256];
    sprintf(str,"%3.1f %s", val, units);

    return std::string(str);
}

JThreadManager* JApplication::GetJThreadManager() const {
    return _threadManager;
}

void JApplication::UpdateResourceLimits() {

    /// Used internally by JANA to adjust the maximum size of resource
    /// pools after changing the number of threads.

    // OK, this is tricky. The max size of the JFactorySet resource pool should
    // be at least as big as how many threads we have. Factory sets may also be
    // attached to JEvent objects in queues that are not currently being acted
    // on by a thread so we actually need the maximum to be larger if we wish to
    // prevent constant allocation/deallocation of factory sets. If the factory
    // sets are large, this will cost more memory, but should save on CPU from
    // allocating less often and not having to call the constructors repeatedly.
    // The exact maximum is hard to determine here. We set it to twice the number
    // of threads which should be sufficient. The user should be given control to
    // adjust this themselves in the future, but or now, this should be OK.
    auto nthreads = _threadManager->GetNJThreads();
    mFactorySetPool.Set_ControlParams( nthreads*100, 10 );
}


