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

#include <unistd.h>
#include <dlfcn.h>
#include <sched.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_set>
using namespace std;

#include "JApplication.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JQueueSimple.h>
#include <JANA/JParameterManager.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JThreadManager.h>
#include <JANA/JThread.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>
#include <JANA/JVersion.h>
#include <JANA/JResourcePool.h>
#include <JANA/JResourcePoolSimple.h>
#include <JANA/JCpuInfo.h>

JApplication *japp = NULL;



//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(JParameterManager* params,
						   std::vector<string>* eventSources)
{
	_exit_code = 0;
	_quitting = false;
	_draining_queues = false;
	_ticker_on = true;
	mNumProcessorsAdded = 0;

	_pmanager = (params == nullptr) ? new JParameterManager() : params;
	_logger = std::shared_ptr<JLogger>(new JLogger());
	_eventSourceManager = new JEventSourceManager(this);
	_threadManager = new JThreadManager(this);

	if (eventSources != nullptr) {
		for (string & e : *eventSources) {
			LOG_INFO(_logger) << "Adding source: " << e << "\n" << LOG_END;
			_eventSourceManager->AddEventSource(e);
		}
	}
}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{
	for( auto p: _factoryGenerators     ) delete p;
	for( auto p: _eventProcessors       ) delete p;
	if( _pmanager           ) delete _pmanager;
	if( _threadManager      ) delete _threadManager;
	if( _eventSourceManager ) delete _eventSourceManager;
}

//---------------------------------
// AttachPlugins
//---------------------------------
void JApplication::AttachPlugins(void)
{
	/// Loop over list of plugin names added via AddPlugin() and
	/// actually attach and intiailize them. See AddPlugin method
	/// for more.

	bool printPaths = false;
	GetJParameterManager()->SetDefaultParameter(
			"JANA:DEBUG_PLUGIN_LOADING",
			printPaths,
			"Trace the plugin search path and display any loading errors");

	// In order to give priority to factories added via plugins,
	// the list of factory generators needs to be cleared so
	// those added from plugins will be at the front of the list.
	// We make a copy of the existing generators first so we can
	// append them back to the end of the list before exiting.
	// Similarly for event source generators and calibration generators.
	vector<JEventSourceGenerator*> my_eventSourceGenerators;
	_eventSourceManager->GetJEventSourceGenerators(my_eventSourceGenerators);
	vector<JFactoryGenerator*> my_factoryGenerators = _factoryGenerators;
	vector<JCalibrationGenerator*> my_calibrationGenerators = _calibrationGenerators;
	_eventSourceManager->ClearJEventSourceGenerators();
	_factoryGenerators.clear();
	_calibrationGenerators.clear();

	// The JANA_PLUGIN_PATH specifies directories to search
	// for plugins that were explicitly added through AddPlugin(...).
	// Multiple directories can be specified using a colon(:) separator.
	const char *jpp = getenv("JANA_PLUGIN_PATH");
	if(jpp){
		stringstream ss(jpp);
		string path;
		while(getline(ss, path, ':')) AddPluginPath( path );
	}

	// Default plugin search path
	AddPluginPath(".");
	if(const char *ptr = getenv("JANA_HOME")) AddPluginPath(string(ptr) + "/plugins");

	// Add plugins specified via PLUGINS configuration parameter
	// (comma separated list).
	try{
		string plugins;
		GetJParameterManager()->GetParameter("PLUGINS", plugins);
		stringstream ss(plugins);
		string p;
		while(getline(ss, p, ',')) _plugins.push_back(p);
	}catch(...){
		LOG_FATAL(_logger) << "Unknown exception while parsing PLUGINS parameter" << LOG_END;
		SetExitCode(-1);
		Quit();
	}

	// Loop over plugins
	stringstream err_mess;
	for(string plugin : _plugins){
		// Sometimes, the user will include the ".so" suffix in the
		// plugin name. If they don't, then we add it here.
		if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";

		// Loop over paths
		bool found_plugin=false;
		for(string path : _plugin_paths){
			string fullpath = path + "/" + plugin;
			LOG_TRACE(_logger, printPaths) << "Looking for '" << fullpath << "' ...." << LOG_END;
			if (access(fullpath.c_str(), F_OK) != -1) {
				LOG_TRACE(_logger, printPaths) << "Found!" << LOG_END;
				try{
					AttachPlugin(fullpath.c_str(), printPaths);
					found_plugin=true;
					break;
				} catch(...) {
					err_mess << "Tried to attach: \"" << fullpath << "\"" << endl;
					err_mess << "  -- error message: " << dlerror() << endl;
					continue;
				}
			}
			LOG_TRACE(_logger, printPaths) << "Failed to attach '" << fullpath << "'" << LOG_END;
		}

		// If we didn't find the plugin, then complain and quit
		if(!found_plugin){

			LOG_FATAL(_logger) << "\n*** Couldn't find plugin '" << plugin << "'! ***\n" <<
							   "***        Make sure the JANA_PLUGIN_PATH environment variable is set correctly.\n" <<
							   "***        To see paths checked, set JANA:DEBUG_PLUGIN_LOADING=1\n"<<
							   "***        Some hints to the error follow:\n\n" << err_mess.str() << LOG_END;

			exit(-1);
		}
	}

	// Append generators back onto appropriate lists
	for(auto sGenerator : my_eventSourceGenerators)
		_eventSourceManager->AddJEventSourceGenerator(sGenerator);
	_factoryGenerators.insert(_factoryGenerators.end(), my_factoryGenerators.begin(), my_factoryGenerators.end());
	_calibrationGenerators.insert(_calibrationGenerators.end(), my_calibrationGenerators.begin(), my_calibrationGenerators.end());
}

//---------------------------------
// AttachPlugin
//---------------------------------
void JApplication::AttachPlugin(string soname, bool verbose)
{
	/// Attach a plugin by opening the shared object file and running the
	/// InitPlugin_t(JApplication* app) global C-style routine in it.
	/// An exception will be thrown if the plugin is not successfully opened.
	/// Users will not need to call this directly since it is called automatically
	/// from Initialize().
	///
	/// @param soname name of shared object file to attach. This may include
	///               an absolute or relative path.
	///
	/// @param verbose if set to true, failed attempts will be recorded via the
	///                JLog. Default is false so JANA can silently ignore files
	///                that are not valid plugins.
	///

	// Open shared object
	void* handle = dlopen(soname.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
	if(!handle){
		LOG_TRACE(_logger, verbose) << dlerror() << LOG_END;
		throw "dlopen failed";
	}

	// Look for an InitPlugin symbol
	typedef void InitPlugin_t(JApplication* app);
	InitPlugin_t *plugin = (InitPlugin_t*)dlsym(handle, "InitPlugin");
	if(plugin){
		LOG_INFO(_logger) << "Initializing plugin \"" << soname << "\"" << LOG_END;
		(*plugin)(this);
		_sohandles.push_back(handle);
	}else{
		dlclose(handle);
		LOG_TRACE(_logger, verbose) << "Nothing useful found in " << soname << LOG_END;
	}
}

//---------------------------------
// AddPlugin
//---------------------------------
void JApplication::AddPlugin(string plugin_name)
{
	/// Add the specified plugin to the list of plugins to be
	/// attached. This only records the name. The plugin is not
	/// actually attached until AttachPlugins() is called (typically
	/// from Initialize() which is called from Run()).
	/// This will check if the plugin already exists in the list
	/// of plugins to attach and will not add it a second time
	/// if it is already there. This may be important if the order
	/// of plugins is important. It is left to the user to handle
	/// in those cases.
	///
	/// @param plugin_name name of the plugin. Do not include the
	///                    ".so" or ".dylib" suffix in the name.
	///                    The path to the plugin will be searched
	///                    from the JANA_PLUGIN_PATH envar.
	///
	for( string &n : _plugins) if( n == plugin_name ) return;
	_plugins.push_back(plugin_name);
}

//---------------------------------
// AddPluginPath
//---------------------------------
void JApplication::AddPluginPath(string path)
{
	/// Add a path to the directories searched for plugins. This
	/// should not include the plugin name itself. This only has
	/// an effect when called before AttachPlugins is called
	/// (i.e. before Run is called).
	/// n.b. if this is called with a path already in the list,
	/// then the call is silently ignored.
	///
	/// Generally, users will set the path via the JANA_PLUGIN_PATH
	/// environment variable and won't need to call this method. This
	/// may be called if it needs to be done programmatically.
	///
	/// @param path directory to search fpr plugins.
	///
	for( string &n : _plugin_paths) if( n == path ) return;
	_plugin_paths.push_back(path);
}

//---------------------------------
// GetExitCode
//---------------------------------
int JApplication::GetExitCode(void)
{
	/// Returns the currently set exit code. This can be used by
	/// JProcessor/JFactory classes to communicate an appropriate
	/// exit code that a jana program can return upon exit. The
	/// value can be set via the SetExitCode method.

	return _exit_code;
}

//---------------------------------
// Initialize
//---------------------------------
void JApplication::Initialize(void)
{
	/// Initialize the application in preparation for data processing.
	/// This is called by the Run method so users will usually not
	/// need to call this directly.

	if (_initialized) return;

	// Set number of threads
	_nthreads = JCpuInfo::GetNumCpus();
	_pmanager->SetDefaultParameter("NTHREADS", _nthreads, "The total number of worker threads");

	// Set task pool size
	int task_pool_size = 200;
	int task_pool_debuglevel = 0;
	_pmanager->SetDefaultParameter("JANA:TASK_POOL_SIZE", task_pool_size, "Task pool size");
	_pmanager->SetDefaultParameter("JANA:TASK_POOL_DEBUGLEVEL", task_pool_debuglevel, "Task pool debug level");
	mVoidTaskPool.Set_ControlParams(task_pool_size, task_pool_debuglevel);

	// Attach all plugins
	AttachPlugins();

	// Create all event sources
	_eventSourceManager->CreateSources();

	// Get factory generators from event sources
	std::deque<JEventSource*> sEventSources;
	_eventSourceManager->GetUnopenedJEventSources(sEventSources);
	std::unordered_set<std::type_index> sSourceTypes;
	for(auto sSource : sEventSources)
	{
		auto sTypeIndex = sSource->GetDerivedType();
		if(sSourceTypes.find(sTypeIndex) != std::end(sSourceTypes))
			continue; //same type as before: duplicate factories!

		auto sGenerator = sSource->GetFactoryGenerator();
		if(sGenerator != nullptr)
			_factoryGenerators.push_back(sGenerator);
	}

	//Prepare for running: Open event sources and prepare task queues for them
	_eventSourceManager->OpenInitSources();
	_threadManager->PrepareQueues();
}

//---------------------------------
// PrintFinalReport
//---------------------------------
void JApplication::PrintFinalReport(void)
{
	//Get queues
	std::vector<JThreadManager::JEventSourceInfo*> sRetiredQueues;
	_threadManager->GetRetiredSourceInfos(sRetiredQueues);
	std::vector<JThreadManager::JEventSourceInfo*> sActiveQueues;
	_threadManager->GetActiveSourceInfos(sActiveQueues);
	auto sAllQueues = sRetiredQueues;
	sAllQueues.insert(sAllQueues.end(), sActiveQueues.begin(), sActiveQueues.end());


	// Get longest JQueue name
	uint32_t sSourceMaxNameLength = 0, sQueueMaxNameLength = 0;
	for(auto& sSourceInfo : sAllQueues)
	{
		auto sSource = sSourceInfo->mEventSource;
		auto sSourceLength = sSource->GetName().size();
		if(sSourceLength > sSourceMaxNameLength)
			sSourceMaxNameLength = sSourceLength;

		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
		sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
		for(auto& sTypePair : sSourceQueues)
		{
			for(auto sQueue : sTypePair.second)
			{
				auto sLength = sQueue->GetName().size();
				if(sLength > sQueueMaxNameLength)
					sQueueMaxNameLength = sLength;
			}
		}
	}
	sSourceMaxNameLength += 2;
	if(sSourceMaxNameLength < 8) sSourceMaxNameLength = 8;
	sQueueMaxNameLength += 2;
	if(sQueueMaxNameLength < 7) sQueueMaxNameLength = 7;

	jout << std::endl;
	jout << "Final Report" << std::endl;
	jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
	jout << "Source" << std::string(sSourceMaxNameLength - 6, ' ') << "   Nevents  " << "Queue" << std::string(sQueueMaxNameLength - 5, ' ') << "NTasks" << std::endl;
	jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
	std::size_t sSrcIdx = 0;
	for(auto& sSourceInfo : sAllQueues)
	{
		// Flag to prevent source name and number of events from
		// printing more than once.
		bool sSourceNamePrinted = false;

		// Place "*" next to names of active sources
		string sFlag;
		if( sSrcIdx++ >= sRetiredQueues.size() ) sFlag="*";

		auto sSource = sSourceInfo->mEventSource;
		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
		sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
		for(auto& sTypePair : sSourceQueues)
		{
			for(auto sQueue : sTypePair.second)
			{
				string sSourceName = sSource->GetName()+sFlag;

				if( sSourceNamePrinted ){
					jout << string(sSourceMaxNameLength + 12, ' ');
				}else{
					sSourceNamePrinted = true;
					jout << sSourceName << string(sSourceMaxNameLength - sSourceName.size(), ' ')
						 << std::setw(10) << sSource->GetNumEventsProcessed() << "  ";
				}

				jout << sQueue->GetName() << string(sQueueMaxNameLength - sQueue->GetName().size(), ' ')
					 << sQueue->GetNumTasksProcessed() << std::endl;
			}
		}
	}
	if( !sActiveQueues.empty() ){
		jout << std::endl;
		jout << "(*) indicates sources that were still active" << std::endl;
	}

	jout << std::endl;
	jout << "Total events processed: " << GetNeventsProcessed() << " (~ " << Val2StringWithPrefix( GetNeventsProcessed() ) << "evt)" << std::endl;
	jout << "Integrated Rate: " << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz" << std::endl;
	jout << std::endl;

	// Optionally print more info if user requested it:
	bool print_extended_report = false;
	_pmanager->SetDefaultParameter("JANA:EXTENDED_REPORT", print_extended_report);
	if( print_extended_report ){
		jout << std::endl;
		jout << "Extended Report" << std::endl;
		jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
		jout << "               Num. plugins: " << _plugins.size() <<std::endl;
		jout << "          Num. plugin paths: " << _plugin_paths.size() <<std::endl;
		jout << "    Num. factory generators: " << _factoryGenerators.size() <<std::endl;
		jout << "Num. calibration generators: " << _calibrationGenerators.size() <<std::endl;
		jout << "      Num. event processors: " << mNumProcessorsAdded <<std::endl;
		jout << "          Num. factory sets: " << mFactorySetPool.Get_PoolSize() << " (max. " << mFactorySetPool.Get_MaxPoolSize() << ")" << std::endl;
		jout << "       Num. config. params.: " << _pmanager->GetNumParameters() <<std::endl;
		jout << "               Num. threads: " << _threadManager->GetNJThreads() <<std::endl;
	}
}

//---------------------------------
// PrintStatus
//---------------------------------
void JApplication::PrintStatus(void)
{
	// Print ticker
	stringstream ss;
	ss << "  " << GetNeventsProcessed() << " events processed  " << Val2StringWithPrefix( GetInstantaneousRate() ) << "Hz (" << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz avg)             ";
	jout << ss.str() << "\r";
	jout.flush();
}

//---------------------------------
// Quit
//---------------------------------
void JApplication::Quit(bool skip_join)
{
	_threadManager->EndThreads();
	_skip_join = skip_join;
	_quitting = true;

}

//---------------------------------
// Run
//---------------------------------
void JApplication::Run()
{

	// Setup all queues and attach plugins
	Initialize();

	// If something went wrong in Initialize() then we may be quitting early.
	if(_quitting) return;

	// Create all remaining threads (one may have been created in Init)
	LOG_INFO(_logger) << "Creating " << _nthreads << " processing threads ..." << LOG_END;
	_threadManager->CreateThreads(_nthreads);

	// Optionally set thread affinity
	try{
		int affinity_algorithm = 0;
		_pmanager->SetDefaultParameter("AFFINITY", affinity_algorithm, "Set the thread affinity algorithm. 0=none, 1=sequential, 2=core fill");
		_threadManager->SetThreadAffinity( affinity_algorithm );
	}catch(...){
		LOG_ERROR(_logger) << "Unknown exception in JApplication::Run attempting to set thread affinity" << LOG_END;
	}

	// Print summary of all config parameters (if any aren't default)
	GetJParameterManager()->PrintParameters(true);

	// Start all threads running
	jout << "Start processing ..." << endl;
	mRunStartTime = std::chrono::high_resolution_clock::now();
	_threadManager->RunThreads();

	// Monitor status of all threads
	while( !_quitting ){

		// If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
		// This flag is used by the integrated rate calculator
		// The JThreadManager is in charge of telling all the threads to end
		if(!_draining_queues)
			_draining_queues = _eventSourceManager->AreAllFilesClosed();

		// Check if all threads have finished
		if(_threadManager->AreAllThreadsEnded())
		{
			std::cout << "All threads have ended.\n";
			break;
		}

		// Sleep a few cycles
		std::this_thread::sleep_for( std::chrono::milliseconds(500) );

		// Print status
		if( _ticker_on ) PrintStatus();
	}

	// Join all threads
	jout << "Event processing ended. " << endl;
	if (!_skip_join) {
		cout << "Merging threads ..." << endl;
		_threadManager->JoinThreads();
	}

	// Delete event processors
	for(auto sProcessor : _eventProcessors){
		sProcessor->Finish(); // (this may not be necessary since it is always next to destructor)
		delete sProcessor;
	}
	_eventProcessors.clear();

	// Report Final numbers
	PrintFinalReport();
}

//---------------------------------
// SetExitCode
//---------------------------------
void JApplication::SetExitCode(int exit_code)
{
	/// Set a value of the exit code in that can be later retrieved
	/// using GetExitCode. This is so the executable can return
	/// a meaningful error code if processing is stopped prematurely,
	/// but the program is able to stop gracefully without a hard 
	/// exit. See also GetExitCode.

	_exit_code = exit_code;
}

//---------------------------------
// SetTicker
//---------------------------------
void JApplication::SetTicker(bool ticker_on)
{
	_ticker_on = ticker_on;
}

//---------------------------------
// Stop
//---------------------------------
void JApplication::Stop(bool wait_until_idle)
{
	/// Tell all JThread objects to go into the idle state after completing
	/// their current task. If wait_until_idle is true, this will block until
	/// all threads are in the idle state. If false (the default), it will return
	/// immediately after telling all threads to go into the idle state
	_threadManager->StopThreads(wait_until_idle);
}

//---------------------------------
// Resume
//---------------------------------
void JApplication::Resume(void)
{
	/// Tell all JThread objects to go into the running state (if not already
	/// there).
	_threadManager->RunThreads();
}

//---------------------------------
// Add - JEventSourceGenerator
//---------------------------------
void JApplication::Add(JEventSourceGenerator *source_generator)
{
	/// Add the given JFactoryGenerator to the list of queues
	///
	/// @param source_generator pointer to source generator to add. Ownership is passed to JApplication

	GetJEventSourceManager()->AddJEventSourceGenerator( source_generator );
}

//---------------------------------
// Add - JFactoryGenerator
//---------------------------------
void JApplication::Add(JFactoryGenerator *factory_generator)
{
	/// Add the given JFactoryGenerator to the list of queues
	///
	/// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication

	_factoryGenerators.push_back( factory_generator );
}

//---------------------------------
// Add - JEventProcessor
//---------------------------------
void JApplication::Add(JEventProcessor *processor)
{
	_eventProcessors.push_back( processor );
	mNumProcessorsAdded++;
}

//---------------------------------
// GetJEventProcessors
//---------------------------------
void JApplication::GetJEventProcessors(vector<JEventProcessor*>& aProcessors)
{
	aProcessors = _eventProcessors;
}

//---------------------------------
// GetJFactoryGenerators
//---------------------------------
void JApplication::GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators)
{
	factory_generators = _factoryGenerators;
}

//---------------------------------
// GetJLogger
//---------------------------------
std::shared_ptr<JLogger> JApplication::GetJLogger(void)
{
	return _logger;
}


//---------------------------------
// GetJParameterManager
//---------------------------------
JParameterManager* JApplication::GetJParameterManager(void)
{
	/// Return pointer to the JParameterManager object.

	if( !_pmanager ) _pmanager = new JParameterManager();

	return _pmanager;
}


//---------------------------------
// GetJThreadManager
//---------------------------------
JThreadManager* JApplication::GetJThreadManager(void) const
{
	return _threadManager;
}

//---------------------------------
// GetJEventSourceManager
//---------------------------------
JEventSourceManager* JApplication::GetJEventSourceManager(void) const
{
	return _eventSourceManager;
}


//---------------------------------
// GetVoidTask
//---------------------------------
std::shared_ptr<JTask<void>> JApplication::GetVoidTask(void)
{
	return mVoidTaskPool.Get_SharedResource();
}

//---------------------------------
// GetFactorySet
//---------------------------------
JFactorySet* JApplication::GetFactorySet(void)
{
	return mFactorySetPool.Get_Resource(_factoryGenerators);
}

//---------------------------------
// Recycle
//---------------------------------
void JApplication::Recycle(JFactorySet* aFactorySet)
{
	return mFactorySetPool.Recycle(aFactorySet);
}

//---------------------------------
// UpdateResourceLimits
//---------------------------------
void JApplication::UpdateResourceLimits(void)
{
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

//---------------------------------
// GetNtasksCompleted
//---------------------------------
uint64_t JApplication::GetNtasksCompleted(string name)
{
	return 0;
}

//---------------------------------
// GetNeventsProcessed
//---------------------------------
uint64_t JApplication::GetNeventsProcessed(void)
{
	/// Return the total number of events processed.
	return _eventSourceManager->GetNumEventsProcessed();
}

//---------------------------------
// GetIntegratedRate
//---------------------------------
float JApplication::GetIntegratedRate(void)
{
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

//---------------------------------
// GetInstantaneousRate
//---------------------------------
float JApplication::GetInstantaneousRate(void)
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

//---------------------------------
// GetInstantaneousRates
//---------------------------------
void JApplication::GetInstantaneousRates(vector<double> &rates_by_queue)
{

}

//---------------------------------
// GetIntegratedRates
//---------------------------------
void JApplication::GetIntegratedRates(map<string,double> &rates_by_thread)
{

}

//----------------
// Val2StringWithPrefix
//----------------
string JApplication::Val2StringWithPrefix(float val)
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

	return string(str);
}

