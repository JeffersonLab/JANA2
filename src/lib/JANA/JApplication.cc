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

#include <JANA/JApplication.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>

#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Services/JProcessingController.h>

#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Utils/JCpuInfo.h>

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    if (_params == nullptr) {
        _params = std::make_shared<JParameterManager>();
    }
    else {
        _params = std::shared_ptr<JParameterManager>(params);
    }

    _service_locator.provide(_params);
    _service_locator.provide(std::make_shared<JLoggingService>());
    _service_locator.provide(std::make_shared<JPluginLoader>(this));
    _service_locator.provide(std::make_shared<JComponentManager>(this));

    _plugin_loader = _service_locator.get<JPluginLoader>();
    _component_manager = _service_locator.get<JComponentManager>();

    _logger = JLogger(JLogger::Level::INFO); // TODO: Get from logging service
}


JApplication::~JApplication() {}


// Loading plugins

void JApplication::AddPlugin(std::string plugin_name) {
    _plugin_loader->add_plugin(plugin_name);
}

void JApplication::AddPluginPath(std::string path) {
    _plugin_loader->add_plugin_path(path);
}


// Building a ProcessingTopology

void JApplication::Add(JEventSource* event_source) {
    _component_manager->add(event_source);
}

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param source_generator pointer to source generator to add. Ownership is passed to JApplication
    _component_manager->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    _component_manager->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    _component_manager->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    _component_manager->add(event_source_name);
}


// Controlling processing

void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not
    /// need to call this directly.

    try {
        // Only run this once
        if (_initialized) return;

        // Attach all plugins
        _plugin_loader->attach_plugins(_component_manager.get());

        // Set task pool size
        size_t task_pool_size = 200;
        size_t task_pool_debuglevel = 0;
        _params->SetDefaultParameter("JANA:TASK_POOL_SIZE", task_pool_size, "Task pool size");
        _params->SetDefaultParameter("JANA:TASK_POOL_DEBUGLEVEL", task_pool_debuglevel, "Task pool debug level");

        // Set desired nthreads
        _desired_nthreads = JCpuInfo::GetNumCpus();
        _params->SetDefaultParameter("NTHREADS", _desired_nthreads,
                                     "The total number of worker threads");


        _component_manager->resolve_event_sources();
        auto topology = JArrowTopology::from_components(_component_manager, _params);
        _processing_controller = std::make_shared<JArrowProcessingController>(topology);

        _params->SetDefaultParameter("JANA:EXTENDED_REPORT", _extended_report);
        _processing_controller->initialize();
        _initialized = true;
    }
    catch (JException e) {
        LOG_FATAL(_logger) << e << LOG_END;
        exit(-1);
    }
}

void JApplication::Run(bool wait_until_finished) {

    Initialize();
    if(_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    _params->PrintParameters(false);

    LOG_INFO(_logger) << GetComponentSummary() << LOG_END;
    LOG_INFO(_logger) << "Starting processing ..." << LOG_END;
    mRunStartTime = std::chrono::high_resolution_clock::now();
    _processing_controller->run(_desired_nthreads);

    if (!wait_until_finished) {
        return;
    }

    // Monitor status of all threads
    while( !_quitting ){

        // If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
        // This flag is used by the integrated rate calculator
        // The JThreadManager is in charge of telling all the threads to end
        // TODO: Bring this back
        //if(!_draining_queues)
        //    _draining_queues = _topology->event_source_manager.AreAllFilesClosed();

        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (_processing_controller->is_stopped() || _processing_controller->is_finished()) {
            LOG_INFO(_logger) << "All threads have ended." << LOG_END;
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for( std::chrono::milliseconds(500) );

        // Print status
        if( _ticker_on ) PrintStatus();
    }

    // Join all threads
    if (!_skip_join) {
        LOG_INFO(_logger) << "Merging threads ..." << LOG_END;
        _processing_controller->wait_until_stopped();
    }

    LOG_INFO(_logger) << "Event processing ended." << LOG_END;
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

JComponentSummary JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return _component_manager->get_component_summary();
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
        LOG_INFO(_logger) << "Running: " << GetNeventsProcessed() << " events processed  "
                          << JTypeInfo::to_string_with_si_prefix(GetInstantaneousRate()) << "Hz ("
                          << JTypeInfo::to_string_with_si_prefix(GetIntegratedRate()) << "Hz avg)" << LOG_END;
    }
}

void JApplication::PrintFinalReport() {
    _processing_controller->print_final_report();
}

uint64_t JApplication::GetNThreads() {
    auto perf = _processing_controller->measure_performance();
    return perf->thread_count;
}

uint64_t JApplication::GetNeventsProcessed() {
    auto perf = _processing_controller->measure_performance();
    return perf->monotonic_events_completed;
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


