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
#include <JANA/JParameterManager.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JException.h>
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
	_logger = JLoggingService::logger("JApplication");
	_eventSourceManager = new JEventSourceManager(this);

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
// GetJParameterManager
//---------------------------------
JParameterManager* JApplication::GetJParameterManager(void)
{
	/// Return pointer to the JParameterManager object.

	if( !_pmanager ) _pmanager = new JParameterManager();

	return _pmanager;
}


//---------------------------------
// GetJEventSourceManager
//---------------------------------
JEventSourceManager* JApplication::GetJEventSourceManager(void) const
{
	return _eventSourceManager;
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

uint64_t JApplication::GetNThreads() {
	return _nthreads;
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

