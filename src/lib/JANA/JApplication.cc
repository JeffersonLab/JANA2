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
#include <JANA/JThread.h>
#include <JANA/JTask.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>
#include <JANA/JVersion.h>


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


//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(int narg, char *argv[])
{
	// Set up to catch SIGINTs for graceful exits
	signal(SIGINT,ctrlCHandle);
	
	_exit_code = 0;
	_quitting = false;
	_draining_queues = false;
	_pmanager = NULL;
	_rmanager = NULL;
	_all_sources_opened = false;

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

		_source_names.push_back(arg);
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
	vector<JEventSourceGenerator*> my_eventSourceGenerators = _eventSourceGenerators;
	vector<JFactoryGenerator*> my_factoryGenerators = _factoryGenerators;
	vector<JCalibrationGenerator*> my_calibrationGenerators = _calibrationGenerators;
	_eventSourceGenerators.clear();
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
	_eventSourceGenerators.insert(_eventSourceGenerators.end(), my_eventSourceGenerators.begin(), my_eventSourceGenerators.end());
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
	/// from Run()).
	/// This will check if the plugin already exists in the list
	/// of plugins to attach and will not add it a second time
	/// if it is already there. This may be important if the order
	/// of plugins is important. It is left to the user to handle
	/// in those cases.
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
	AddJQueue( new JQueue("Physics Events") );
	
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
	ss << " " << GetNeventsProcessed() << " events processed  " << Val2StringWithPrefix( GetInstantaneousRate() ) << "Hz (" << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz avg)             ";
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
	
	// Set number of threads
	try{
		string snthreads = GetParameterValue<string>("NTHREADS");
		if( snthreads == "Ncores" ){
			nthreads = GetNcores();
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
	while( _jthreads.size() < nthreads ) _jthreads.push_back( new JThread(this) );
	
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
	
}

//---------------------------------
// AddEventSource
//---------------------------------
void JApplication::AddEventSource(std::string source_name)
{
	/// Add the named event source to the list of event sources to read from.
	/// Usually, these are taken from the command line, but this allows cutom
	/// plugins to add a source programmatically. This will be added to the end
	/// of the current list of source names (i.e. after any entered on the
	/// command line.)
	
	_source_names.push_back(source_name);
}

//---------------------------------
// AddJEventSource
//---------------------------------
void JApplication::AddJEventSource(JEventSource *source)
{
	
}

//---------------------------------
// AddJEventSourceGenerator
//---------------------------------
void JApplication::AddJEventSourceGenerator(JEventSourceGenerator *source_generator)
{
	
}

//---------------------------------
// AddJFactoryGenerator
//---------------------------------
void JApplication::AddJFactoryGenerator(JFactoryGenerator *factory_generator)
{
	
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
void JApplication::GetJEventProcessors(vector<JEventProcessor*> &processors)
{
	
}

//---------------------------------
// GetJEventSources
//---------------------------------
void JApplication::GetJEventSources(vector<JEventSource*> &sources)
{
	
}

//---------------------------------
// GetJEventSourceGenerators
//---------------------------------
void JApplication::GetJEventSourceGenerators(vector<JEventSourceGenerator*> &source_generators)
{
	
}

//---------------------------------
// GetJFactoryGenerators
//---------------------------------
void JApplication::GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators)
{
	
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
// GetNextEvent
//---------------------------------
void JApplication::GetNextEvent(void)
{
	/// Read in a JEvent from one of the active sources. If there are
	/// no active sources, open the next source. If all sources have
	/// been exhausted, then the JApplication will be flagged to quit
	/// and a JException will be thrown.

	// This is designed to allow multiple threads to read from multiple
	// input streams simultaneously. It uses atomics to avoid mutex locking
	// where possible. This does complicate things a bit so here goes an
	// explanation:
	//
	// 1. The _sources_active container is sized to hold the maximum number
	// of simultaneous event streams. Each element will contain either a NULL
	// pointer or a pointer to a valid JEventSource object.
	//
	// 2. This will first look for a non-NULL value in _active_sources. If
	// one is found, it will try and claim exclusive use via its _in_use
	// atomic member. If it obtains that, then it will either read an
	// event from it, or, if there are no more events, set the slot in
	// _actve_sources to NULL indicating it can be filled by another source.
	// The exhausted pointer is then added to the _sources_exhausted container.
	//
	// 3. If exclusive use of a source cannot be obtained and there is at
	// least one NULL pointer in _active_sources, then OpenNext() will be
	// called. That will lock a mutex while creating a new JEventSource
	// and placing it in the _active_sources queue.
	//
	// Mutexes are only used when:
	//
	// A. creating a new JEventSource and writing it into _active_sources
	//
	// B. writing an exhausted JEventSource into _sources_exhausted 
	//
	// Both of those should happen infrequently. Most calls to this should
	// result in either an event being read in or nothing at all happening.
	// Most of the time only accesses to a few atomic variables will occur.
	
	uint32_t islot = 0;
	bool has_null_slot = false;
	for(auto src : _active_sources){
		if( src != nullptr ){
			bool tmp = false;
			if( src->_in_use.compare_exchange_weak(tmp, true) ){
				// We now have exclusive use of src
				if( src->IsDone() ){
					// No more events in this source. Retire it.
					_active_sources[islot] = nullptr;
					lock_guard<mutex> lguard(_sources_exhausted_mutex);
					_sources_exhausted.push_back( src );
				} else {
					try{
						// Read event from this source
						switch( src->GetEvent() ){
							case JEventSource::kSUCCESS:
							case JEventSource::kNO_MORE_EVENTS:
								// Not throwing an exception here will cause the calling
								// JThread to immediately cycle back to start of Loop 
								// without sleeping.
								break;
							case JEventSource::kTRY_AGAIN:
								// This will cause the calling JThread to sleep briefly
								src->_in_use.store( false );
								throw JException("source stalled. try again.");
								break;
						}
					}catch(...){
						// There was a problem reading from this source.
						// Make sure it is flagged as done so it will be
						// cleaned up on a subsequent call.
						src->SetDone(true);
					}
				}
				
				// Free the _in_use flag
				src->_in_use.store( false );
				
				// At this point we have either read in an event or changed
				// the state of a source.
				return;
			}
		}else{
			has_null_slot = true; 
		}
		islot++;
	}
	
	// If we get here then we were not able to get exclusive use of an existing
	// source object. If there are no empty slots then we are waiting on other
	// threads to read in events. Throw an exception to cause the calling JThread
	// to sleep breifly before trying again.
	if( !has_null_slot ) throw JException("thread stalled waiting for event");
	
	// There was and possibly still is at least one empty slot for an event source.
	// Try opening the next source.
	OpenNext();



//	try{
//		switch( OpenNext() ){
//			case kSUCCESS:
//			case kNO_SOURCE_SLOTS_AVAILABLE:
//			case kNO_MORE_SOURCES
//				break;
//			case kNO_MORE_SOURCES:
//		}
//	}catch(JException &e){
//		throw 
//	}

	
	// If all slots in _active_sources are either NULL, or have a source
	// in use by another thread, then OpenNext() will be called. This will

	/// Read the next JEvent from the current event source
	/// or open a new one if needed. The JEventSource object
	/// itself will place the event in the appropriate JQueue.
	/// Note that this may cause new JQueue objects to be
	/// created from the JEventSource subclass constructor.
	/// If no more events are available from the last event
	/// source, 

	// This mutex allows us to manipulate the current source or
	// _sources vector if needed. A source may still implement
	// multi-threaded input (e.g. from multiple independant streams)
	// but would need to do so at the JQueue level rather than
	// the JApplication level.
//	lock_guard<mutex> lg_source(_source_mutex);
//	
//	try{
//		if( _sources.empty() || _sources.back()->IsDone() ) OpenNext();
//		
//		// The following may fail if there are no more events, but
//		// the IsDone flag was not set yet.
//		// It should be the responsibility of the JEventSource subclass
//		// to pick the right queue and place the events in there.
////		_sources.back()->GetEvent();
//		
//	}catch(...){
//		// No more events and no more sources. Set flag to drain
//		// all queues by processing all remaining events and then
//		// quit the program. Throw exception to tell calling JThread
//		// to continue processing events from queues
//		_draining_queues = true;
//		throw JException("No more event sources");	
//	}

//	JQueue *queue = GetJQueue("Physics Events");
//	if( (queue!=NULL) && (queue->GetNumEventsProcessed()<100000000) ){
//		queue->AddToQueue( new JEvent() );
//		return;
//	}
	
}

//---------------------------------
// GetNcores
//---------------------------------
uint32_t JApplication::GetNcores(void)
{
	/// Returns the number of cores that are on the computer.
	/// The count will be full cores+hyperthreads (or equivalent) 

	return sysconf(_SC_NPROCESSORS_ONLN);
}

//---------------------------------
// GetNJThreads
//---------------------------------
uint32_t JApplication::GetNJThreads(void)
{
	/// Returns the number of JThread objects that currently
	/// exist.
	
	return _jthreads.size();
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
// OpenNext
//---------------------------------
void JApplication::OpenNext(void)
{
	/// Try opening the next source in the list. This will return immediately
	/// if all sources have already been opened.


	if( _all_sources_opened ) return;

//	/// Open the next source in the list. If there are none,
//	/// then throw a JException
//	
//	static mutex src_mutex;
//	lock_guard<mutex>(src_mutex);
//
//	if(_sources.size() >= _source_names.size()) throw JException("No more sources");
//	string source_name = _source_names[_sources.size()];
//	current_source = NULL;
//	
//	// Check if the user has forced a specific type of event source
//	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
//	// search for that source and use it. Otherwise, throw an exception.
//	JEventSourceGenerator* gen = NULL;
//	try{
//		string EVENT_SOURCE_TYPE = GetParameterValue<string>("EVENT_SOURCE_TYPE");
//		for( auto sg : _eventSourceGenerators ){
//			if( sg->GetName() == EVENT_SOURCE_TYPE ){
//				gen = sg;
//				jout << "Forcing use of event source type: " << EVENT_SOURCE_TYPE << endl;
//				break;
//			}
//		}
//		if(!gen){
//			jerr << endl;
//			jerr << "-----------------------------------------------------------------" << endl;
//			jerr << " You specified event source type \"" << EVENT_SOURCE_TYPE << "\"" << endl;
//			jerr << " be used to read the event sources but no such type exists." << endl;
//			jerr << " Here is a list of available source types:" << endl;
//			jerr << endl;
//			for( auto sg : _eventSourceGenerators ) jerr << "   " << sg->GetName() << endl;
//			jerr << endl;
//			jerr << "-----------------------------------------------------------------" << endl;
//			SetExitCode(-1);
//			Quit();
//		}
//	}catch(...){}
//	
//	if(!gen){
//	
//		// Loop over JEventSourceGenerator objects and find the one
//		// (if any) that has the highest chance of being able to read
//		// this source. The return value of 
//		// JEventSourceGenerator::CheckOpenable(source) is a liklihood that
//		// the named source can be read by the JEventSource objects
//		// created by the generator. In most cases, the liklihood will
//		// be either 0.0 or 1.0. In the case that 2 or more generators return
//		// equal liklihoods, the first one in the list will be used.
//		double liklihood = 0.0;
//		for( auto sg : _eventSourceGenerators ){
//			double my_liklihood = sg->CheckOpenable(source_name);
//			if(my_liklihood > liklihood){
//				liklihood = my_liklihood;
//				gen = sg;
//			}
//		}
//	}
//	
//	if(gen != NULL){
//		jout << "Opening source \"" << source_name << "\" of type: "<< gen->Description() << endl;
//		current_source = gen->MakeJEventSource(source_name);
//	}
//
//	if(!current_source){
//		jerr<<endl;
//		jerr<<"  xxxxxxxxxxxx  Unable to open event source \""<<source_name<<"\"!  xxxxxxxxxxxx"<<endl;
//		jerr<<endl;
//		if(sources.size()+1 == source_names.size()){
//			unsigned int Nnull_sources = 0;
//			for(unsigned int i=0; i<sources.size(); i++){
//				if(sources[i] == NULL)Nnull_sources++;
//			}
//			if( (Nnull_sources==sources.size()) && (Nsources_deleted==0) ){
//				jerr<<"   xxxxxxxxxxxx  NO VALID EVENT SOURCES GIVEN !!!   xxxxxxxxxxxx  "<<endl;
//				jerr<<endl;
//				SetExitCode(-1);
//				Quit();
//			}
//		}
//	}
//	
//	// Add source to list (even if it's NULL!)
//	sources.push_back(current_source);
}

//---------------------------------
// RemoveJEventProcessor
//---------------------------------
void JApplication::RemoveJEventProcessor(JEventProcessor *processor)
{
	
}

//---------------------------------
// RemoveJEventSource
//---------------------------------
void JApplication::RemoveJEventSource(JEventSource *source)
{
	
}

//---------------------------------
// RemoveJEventSourceGenerator
//---------------------------------
void JApplication::RemoveJEventSourceGenerator(JEventSourceGenerator *source_generator)
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

