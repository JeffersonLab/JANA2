//
//    File: JApplication.cc
// Created: Wed Oct 11 13:09:35 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
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
#include <signal.h>
#include <sched.h>

#ifdef __APPLE__
#import <mach/thread_act.h>
#include <cpuid.h>
#endif  // __APPLE__

#include <cstdlib>
#include <sstream>
#include <iostream>
using namespace std;

#include "JApplication.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JQueue.h>
#include <JANA/JParameterManager.h>
#include <JANA/JResourceManager.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JThreadManager.h>
#include <JANA/JThread.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>
#include <JANA/JVersion.h>
#include <JANA/JStatus.h>
#include <JANA/JResourcePool.h>


JApplication *japp = NULL;

int SIGINT_RECEIVED = 0;
std::mutex DBG_MUTEX;

//-----------------------------------------------------------------
// ctrlCHandle
//-----------------------------------------------------------------
void ctrlCHandle(int x)
{
	SIGINT_RECEIVED++;
	jerr<<endl<<"SIGINT received ("<<SIGINT_RECEIVED<<")....."<<endl;
	
	if(japp) japp->Quit();
	
	if(SIGINT_RECEIVED == 3){
		jerr<<endl<<"Three SIGINTS received! Still attempting graceful exit ..."<<endl<<endl;
	}
	if(SIGINT_RECEIVED == 6){
		jerr<<endl<<"Six SIGINTS received! OK, I get it! ..."<<endl<<endl;
		exit(-2);
	}
}

//-----------------------------------------------------------------
// USR1Handle
//-----------------------------------------------------------------
void USR1Handle(int x)
{
	thread th( JStatus::Report );
	th.detach();
}

//-----------------------------------------------------------------
// USR2Handle
//-----------------------------------------------------------------
void USR2Handle(int x)
{
	JStatus::RecordBackTrace();
}


//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(int narg, char *argv[])
{
	// Set up to catch SIGINTs for graceful exits
	signal(SIGINT,ctrlCHandle);

	// Set up to catch USR1's and USR2's for status reporting
	signal(SIGUSR1,USR1Handle);
	signal(SIGUSR2,USR2Handle);
	
	_exit_code = 0;
	_quitting = false;
	_draining_queues = false;
	_pmanager = NULL;
	_rmanager = NULL;
	_eventSourceManager = new JEventSourceManager();
	_threadManager = new JThreadManager(_eventSourceManager);
	mVoidTaskPool = std::make_shared<JResourcePool<JTask<void>>>();
	mVoidTaskPool->Set_ControlParams(30, 20, 200, 100, 0);

	// Loop over arguments
	if(narg>0) _args.push_back(string(argv[0]));
	for(int i=1; i<narg; i++){
	
		string arg  = argv[i];
		string next = (i+1)<narg ? argv[i+1]:"";
	
		// Record arguments
		_args.push_back( arg );
	
//		arg="--config=";
//		if(!strncmp(arg, argv[i],strlen(arg))){
//			string fname(&argv[i][strlen(arg)]);
//			jparms->ReadConfigFile(fname);
//			continue;
//		}
//		arg="--dumpcalibrations";
//		if(!strncmp(arg, argv[i],strlen(arg))){
//			dump_calibrations = true;
//			continue;
//		}
//		arg="--dumpconfig";
//		if(!strncmp(arg, argv[i],strlen(arg))){
//			dump_configurations = true;
//			continue;
//		}
//		arg="--listconfig";
//		if(!strncmp(arg, argv[i],strlen(arg))){
//			list_configurations = true;
//			continue;
//		}
//		arg="--resourcereport";
//		if(!strncmp(arg, argv[i],strlen(arg))){
//			print_resource_report = true;
//			continue;
//		}
		if( arg.find("-P") == 0 ){
			auto pos = arg.find("=");
			if( (pos!= string::npos) && (pos>2) ){
				string key = arg.substr(2, pos-2);
				string val = arg.substr(pos+1);
				GetJParameterManager()->SetParameter(key, val);
			}else{
				_DBG_ << " bad parameter argument (" << arg << ") should be of form -Pkey=value" << endl;
			}
			continue;
		}
		if( arg == "--janaversion" ) {
			cout<<"          JANA version: "<<JVersion::GetVersion()<<endl;
			cout<<"        JANA ID string: "<<JVersion::GetIDstring()<<endl;
			cout<<"     JANA SVN revision: "<<JVersion::GetRevision()<<endl;
			cout<<"JANA last changed date: "<<JVersion::GetDate()<<endl;
			cout<<"              JANA URL: "<<JVersion::GetSource()<<endl;
			continue;
		}
		if( arg.find("-") == 0 )continue;

		_eventSourceManager->AddEventSource(arg);
	}
	
	japp = this;
}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{

}

//---------------------------------
// AttachPlugins
//---------------------------------
void JApplication::AttachPlugins(void)
{
	/// Loop over list of plugin names added via AddPlugin() and
	/// actually attach and intiailize them. See AddPlugin method
	/// for more.
	
	bool printPaths=false;
	try{
		GetJParameterManager()->GetParameter("PRINT_PLUGIN_PATHS", printPaths);
	}catch(...){}
	
	// In order to give priority to factories added via plugins,
	// the list of factory generators needs to be cleared so
	// those added from plugins will be at the front of the list.
	// We make a copy of the existing generators first so we can
	// append them back to the end of the list before exiting.
	// Similarly for event source generators and calibration generators.
	vector<JEventSourceGenerator*> my_eventSourceGenerators = _eventSourceManager->GetJEventSourceGenerators();
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
	}catch(...){}
	
	// Loop over plugins
	for(string plugin : _plugins){
		// Sometimes, the user will include the ".so" suffix in the
		// plugin name. If they don't, then we add it here.
		if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";
	
		// Loop over paths
		bool found_plugin=false;
		for(string path : _plugin_paths){
			string fullpath = path + "/" + plugin;
			if(printPaths) jout<<"Looking for \""<<fullpath<<"\" ...."; cout.flush();
			if( access( fullpath.c_str(), F_OK ) != -1 ){
				if(printPaths) jout<<"found." << endl;
				try{
					AttachPlugin(fullpath.c_str(), printPaths);
					found_plugin=true;
					break;
				}catch(...){
					continue;
				}
			}
			if(printPaths) jout<<"Failed to attach \""<<fullpath<<"\""<<endl;
		}
		
		// If we didn't find the plugin, then complain and quit
		if(!found_plugin){
			jerr<<endl<<"***ERROR : Couldn't find plugin \""<<plugin<<"\"!***"<<endl;
			jerr<<"***        make sure the JANA_PLUGIN_PATH environment variable is set correctly."<<endl;
			jerr<<"***        To see paths checked, set PRINT_PLUGIN_PATHS config. parameter"<<endl;
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
	// Open shared object
	void* handle = dlopen(soname.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
	if(!handle){
		if(verbose)jerr<<dlerror()<<endl;
		return;
	}
	
	// Look for an InitPlugin symbol
	typedef void InitPlugin_t(JApplication* app);
	InitPlugin_t *plugin = (InitPlugin_t*)dlsym(handle, "InitPlugin");
	if(plugin){
		jout<<"Initializing plugin \""<<soname<<"\" ..."<<endl;
		(*plugin)(this);
		_sohandles.push_back(handle);
	}else{
		dlclose(handle);
		if(verbose)jout<<" --- Nothing useful found in "<<soname<<" ---"<<endl;
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
	for( string &n : _plugins) if( n == plugin_name ) return;
_DBG_<<"Adding plugin " << plugin_name << endl;
	_plugins.push_back(plugin_name);
}

//---------------------------------
// AddPluginPath
//---------------------------------
void JApplication::AddPluginPath(string path)
{
	/// Add a path to the directories searched for plugins. This
	/// should not include the plugin name itself. This only has
	/// an effect when called before AttchPlugins is called
	/// (i.e. before Run is called).
	/// n.b. if this is called with a path already in the list,
	/// then the call is silently ignored.
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
	// Create default JQueue for event processing
	// (plugins may replace this)
	JQueue *physics_queue = new JQueue("Physics Events");
	AddJQueue( physics_queue );
	
	// Attach all plugins
	AttachPlugins();
}

//---------------------------------
// PrintFinalReport
//---------------------------------
void JApplication::PrintFinalReport(void)
{
	// Get longest JQueue name
	uint32_t max_len = 0 ;
	for(auto j : _jqueues) if( j->GetName().length() > max_len ) max_len = j->GetName().length();
	max_len += 2;

	jout << endl;
	jout << "Final Report" << endl;
	jout << "-----------------------------" << endl;
	jout << string(max_len-6, ' ') << "JQueue:  Nevents" << endl;
	jout << " " << string(max_len, '-') << "  --------" <<endl;
	for(auto j : _jqueues){
		string name = j->GetName();
		jout << string(max_len - name.length(), ' ') << name << "  " << j->GetNumEventsProcessed() << endl;
	}
	jout << endl;
	jout << "Integrated Rate: " << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz" << endl;
	jout << endl;

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
void JApplication::Quit(void)
{
	for(auto t : _jthreads ) t->End();
	_quitting = true;
}

//---------------------------------
// Run
//---------------------------------
void JApplication::Run(uint32_t nthreads)
{
	//Do this while still single-threaded!
	_eventSourceManager->OpenInitSources();
	
	// Set number of threads
	try{
		string snthreads = GetParameterValue<string>("NTHREADS");
		if( snthreads == "Ncores" ){
			nthreads = _threadManager->GetNcores();
		}else{
			stringstream ss(snthreads);
			ss >> nthreads;
		}
	}catch(...){}
	if( nthreads==0 ) nthreads=1;

	// Setup all queues and attach plugins
	Initialize();

	// Create all remaining threads (one may have been created in Init)
	jout << "Creating " << nthreads << " processing threads ..." << endl;
	_threadManager->CreateThreads(nthreads);
	
	// Optionally set thread affinity
	try{
		int affinity_algorithm = 0;
		_pmanager->SetDefaultParameter("AFFINITY", affinity_algorithm, "Set the thread affinity algorithm. 0=none, 1=sequential, 2=core fill");
		_threadManager->SetThreadAffinity( affinity_algorithm );
	}catch(...){}
	
	// Print summary of config. parameters (if any aren't default)
	GetJParameterManager()->PrintParameters();

	// Start all threads running
	jout << "Start processing ..." << endl;
	for(auto t : _jthreads ) t->Run();
	
	// Monitor status of all threads
	while( !_quitting ){
		
		// If we are finishing up (i.e. waiting for all events
		// to finish processing) and all JQueues are empty,
		// then tell all threads to quit.
		if( _draining_queues && GetAllQueuesEmpty() ) Quit();
		
		// Check if all threads have finished
		uint32_t Nended = 0;
		for(auto t : _jthreads )if( t->IsEnded() ) Nended++;
		if( Nended == _jthreads.size() ) break;

		// Sleep a few cycles
		this_thread::sleep_for( chrono::milliseconds(500) );
		
		// Print status
		PrintStatus();
	}
	
	// Join all threads
	jout << "Event processing ended. " << endl;
	if( SIGINT_RECEIVED <= 1 ){
		cout << "Merging threads ..." << endl;
		for(auto t : _jthreads ) t->Join();
	}
	
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
// SetMaxThreads
//---------------------------------
void JApplication::SetMaxThreads(uint32_t)
{
	
}

//---------------------------------
// SetTicker
//---------------------------------
void JApplication::SetTicker(bool ticker_on)
{
	
}



//---------------------------------
// Stop
//---------------------------------
void JApplication::Stop(void)
{
	
}

//---------------------------------
// AddJEventProcessor
//---------------------------------
void JApplication::AddJEventProcessor(JEventProcessor *processor)
{
	_eventProcessors.push_back( processor );
}

//---------------------------------
// AddJFactoryGenerator
//---------------------------------
void JApplication::AddJFactoryGenerator(JFactoryGenerator *factory_generator)
{
	/// Add the given JFactoryGenerator to the list of queues

	_factoryGenerators.push_back( factory_generator );
}

//---------------------------------
// AddJQueue
//---------------------------------
void JApplication::AddJQueue(JQueue *queue)
{
	/// Add the given JQueue to the list of queues

	_jqueues.push_back( queue );
}

//---------------------------------
// GetJEventProcessors
//---------------------------------
void JApplication::GetJEventProcessors(vector<JEventProcessor*>& aProcessors)
{
	_eventProcessors = aProcessors;
}

//---------------------------------
// GetJFactoryGenerators
//---------------------------------
void JApplication::GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators)
{
	factory_generators = _factoryGenerators;
}

//---------------------------------
// GetJQueues
//---------------------------------
void JApplication::GetJQueues(vector<JQueue*> &queues)
{
	/// Copy list of the pointers to all JQueue objects into
	/// provided container.
	
	queues = _jqueues;
}

//---------------------------------
// GetJQueue
//---------------------------------
JQueue* JApplication::GetJQueue(const string &name)
{
	/// Return pointer to the JQueue object with the given name.
	/// If no such queue exists, NULL is returned.
	
	for(auto q : _jqueues ) if( q->GetName() == name ) return q;
	
	return NULL;
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
// GetJResourceManager
//---------------------------------
JResourceManager* JApplication::GetJResourceManager(void)
{
	/// Return pointer to the JResourceManager object.

	return _rmanager;
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
// GetAllQueuesEmpty
//---------------------------------
bool JApplication::GetAllQueuesEmpty(void)
{
	/// Returns true if all JQueues are currently empty.
	uint32_t Nempty_queues = 0;
	for( auto q : _jqueues ){
		if( q->GetNumEvents() == 0 ) Nempty_queues++;
	}
	
	return Nempty_queues == _jqueues.size();
}



//---------------------------------
// GetCPU
//---------------------------------
uint32_t JApplication::GetCPU(void)
{
	/// Returns the current CPU the calling thread is running on.
	/// Note that unless the thread affinity has been set, this may 
	/// change, even before returning from this call. The thread
	/// affinity of all threads may be fixed by setting the AFFINITY
	/// configuration parameter at program start up.

	int cpuid;

#ifdef __APPLE__

	//--------- Mac OS X ---------

// From https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
#define CPUID(INFO, LEAF, SUBLEAF) __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])
#define GETCPU(CPU) {                              \
        uint32_t CPUInfo[4];                           \
        CPUID(CPUInfo, 1, 0);                          \
        /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */ \
        if ( (CPUInfo[3] & (1 << 9)) == 0) {           \
          CPU = -1;  /* no APIC on chip */             \
        }                                              \
        else {                                         \
          CPU = (unsigned)CPUInfo[1] >> 24;                    \
        }                                              \
        if (CPU < 0) CPU = 0;                          \
      }

	GETCPU(cpuid);
#else  // __APPLE__

	//--------- Linux ---------
	cpuid = sched_getcpu();

#endif // __APPLE__


	return cpuid;
}

//---------------------------------
// GetVoidTask
//---------------------------------
std::shared_ptr<JTask<void>> JApplication::GetVoidTask(void)
{
	return mVoidTaskPool->Get_SharedResource();
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
	/// Return the total number of events processed. Since
	/// the concept of "event" may be different for each
	/// JQueue when multiple JQueues are present, this will
	/// report the number of events that have passed through
	/// the last JQueue in the chain that has the run_processors
	/// flag set. This should typically be the event count
	/// that the user is most interested in. Note that this
	/// count will include events from the JQueue described
	/// above that are still being processed in a JThread. 
	/// (because those events have technically left the queue.)
	///
	/// If no queues have the run_processor flag set, then the 
	/// number that have left the last queue will be used.
	
	if( _jqueues.empty() ) return 0;
	
	for(auto rit_queue = _jqueues.rbegin(); rit_queue != _jqueues.rend(); rit_queue++){
		if( (*rit_queue)->GetRunProcessors() ) return (*rit_queue)->GetNumEventsProcessed();
	}
	
	return _jqueues.back()->GetNumEventsProcessed();
}

//---------------------------------
// GetIntegratedRate
//---------------------------------
float JApplication::GetIntegratedRate(void)
{

	// Once we start draining the queues, freez the integrated
	// rate so it is not distorted by the wind down
	static float last_R = 0.0;
	if( _draining_queues ) return last_R;

	auto now = std::chrono::high_resolution_clock::now();
	uint64_t N = GetNeventsProcessed();
	static auto start_t = now;
	static uint64_t start_N = N;

	std::chrono::duration<float> delta_t = now - start_t;
	float delta_t_seconds = delta_t.count();
	float delta_N = (float)(GetNeventsProcessed() - start_N);

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

//---------------------------------
// RemoveJEventProcessor
//---------------------------------
void JApplication::RemoveJEventProcessor(JEventProcessor *processor)
{
	
}


//---------------------------------
// RemoveJFactoryGenerator
//---------------------------------
void JApplication::RemoveJFactoryGenerator(JFactoryGenerator *factory_generator)
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

