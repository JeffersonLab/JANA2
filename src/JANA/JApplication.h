// $Id: JApplication.h 1817 2006-06-06 14:59:25Z davidl $
//
//    File: JApplication.h
// Created: Wed Jun  8 12:00:20 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JApplication_
#define _JApplication_

#include <pthread.h>
#include <vector>
#include <string>
#include <list>
#include <utility>
#include <sstream>
using std::vector;
using std::list;
using std::string;
using std::stringstream;

#include "jerror.h"
#include "JParameter.h"
#include "JEventSourceGenerator.h"
#include "JFactoryGenerator.h"
#include "JCalibrationGenerator.h"
#include "JEventLoop.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#ifdef __CINT__
#include "cint.h"
#endif


/// A JANA program will have exactly one JApplication object. It is
/// the central registration point for the other JANA objects.

// Place everything in JANA namespace
namespace jana{

class JEventProcessor;
class JEventSource;
class JEvent;
class JGeometry;
class JParameterManager;
class JCalibration;
class JParameter;
class JEventSourceGenerator;
class JFactoryGenerator;
class JCalibrationGenerator;
class JEventLoop;


class JApplication{
	public:
		JApplication(int narg, char* argv[]); ///< Constructor
		virtual ~JApplication(); ///< Destructor
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JApplication";}
		
		void Usage(void);
		vector<string> GetArgs(void){return args;}
		
		void EventBufferThread(void);
		unsigned int GetEventBufferSize(void);
		virtual jerror_t NextEvent(JEvent &event); ///< Get the next event from the event buffer
		virtual jerror_t ReadEvent(JEvent &event); ///< Get the next event from the source.
		jerror_t AddProcessor(JEventProcessor *processor); ///< Add a JEventProcessor.
		jerror_t RemoveProcessor(JEventProcessor *processor); ///< Remove a JEventProcessor
		jerror_t AddJEventLoop(JEventLoop *loop, double* &heartbeat); ///< Add a JEventLoop
		jerror_t RemoveJEventLoop(JEventLoop *loop); ///< Remove a JEventLoop
		jerror_t AddEventSourceGenerator(JEventSourceGenerator*); ///< Add a JEventSourceGenerator
		jerror_t RemoveEventSourceGenerator(JEventSourceGenerator*); ///< Remove a JEventSourceGenerator
		jerror_t AddFactoryGenerator(JFactoryGenerator*); ///< Add a JFactory Generator
		jerror_t RemoveFactoryGenerator(JFactoryGenerator*); ///< Remove a JFactoryGenerator
		jerror_t AddCalibrationGenerator(JCalibrationGenerator*); ///< Add a JCalibrationGenerator
		jerror_t RemoveCalibrationGenerator(JCalibrationGenerator*); ///< Remove a JCalibrationGenerator
		vector<JEventProcessor*> GetProcessors(void){return processors;} ///< Get the current list of JFactoryGenerators
		vector<JEventLoop*> GetJEventLoops(void){return loops;} ///< Get the current list of JEventLoops
		vector<JEventSourceGenerator*> GetEventSourceGenerators(void){return eventSourceGenerators;} ///< Get the current list of JEventSourceGenerators
		vector<JFactoryGenerator*> GetFactoryGenerators(void){return factoryGenerators;} ///< Get the current list of JFactoryGenerators
		vector<JCalibrationGenerator*> GetCalibrationGenerators(void){return calibrationGenerators;} ///< Get the current list of JCalibrationGenerators
		JParameterManager* GetJParameterManager(void){return jparms;}
		JGeometry* GetJGeometry(unsigned int run_number); ///< Get the JGeometry object for the specified run number.
		JCalibration* GetJCalibration(unsigned int run_number); ///< Get the JCalibration object for the specified run number.
		void GetJCalibrations(vector<JCalibration*> &calibs){calibs=calibrations;} ///< Get the list of existing JCalibration objects
		jerror_t RegisterSharedObject(const char *soname, bool verbose=true); ///< Register a dynamically linked shared object
		jerror_t RegisterSharedObjectDirectory(string sodirname); ///< Register all shared objects in a directory
		jerror_t AddPluginPath(string path); ///< Add a directory to the plugin search path
		jerror_t AddPlugin(const char *name); ///< Add the specified plugin to the shared objects list.
		virtual jerror_t Init(void); ///< Initialize the JApplication object
		virtual jerror_t Run(JEventProcessor *proc=NULL, int Nthreads=0); ///< Process all events from all sources
		virtual jerror_t Fini(void); ///< Gracefully end event processing
		virtual void Pause(void); ///< Pause event processing
		virtual void Resume(void); ///< Resume event processing
		virtual void Quit(void); ///< Stop event processing
		int GetNcores(void){return Ncores;}
		inline int GetNEvents(void){return NEvents;} ///< Returns the number of events processed so far.
		inline float GetRate(void){return rate_instantaneous;} ///< Get the current event processing rate
		const vector<void*> GetSharedObjectHandles(void){return sohandles;} ///< Get pointers to dynamically linked objects
		vector<pair<string,string> > GetAutoActivatedFactories(void){return auto_activated_factories;}
		void AddAutoActivatedFactory(string name, string tag){auto_activated_factories.push_back(pair<string,string>(name,tag));}
		virtual void PrintRate(); ///< Print the current rate to stdout
		void SetShowTicker(int what){show_ticker = what;} ///< Turn auto-printing of rate to screen on or off.
		void SignalThreads(int signo); ///< Send a system signal to all processing threads.
		inline void Lock(void){pthread_mutex_lock(&app_mutex);} ///< Lock the application-wide mutex (don't use this!)
		inline void Unlock(void){pthread_mutex_unlock(&app_mutex);} ///< Unlock the application wide mutex (don't use this!)
		
		bool monitor_heartbeat; ///< Turn monitoring of processing threads on/off.
		
	private:
	
		JApplication(){} ///< Prevent use of default constructor
	
		string Val2StringWithPrefix(float val);
		jerror_t OpenNext(void);
		jerror_t AttachPlugins(void);
		jerror_t RecordFactoryCalls(JEventLoop *loop);
		jerror_t PrintFactoryReport(void);
		jerror_t PrintResourceReport(void);

		bool init_called;
		bool fini_called;
		vector<const char*> source_names;
		vector<JEventSource*> sources;
		JEventSource *current_source;
	
		vector<JEventProcessor*> processors;
		vector<JEventLoop*> loops;
		vector<double*> heartbeats;
		pthread_mutex_t app_mutex;
		map<pthread_t, map<string, unsigned int> > Nfactory_calls;
		map<pthread_t, map<string, unsigned int> > Nfactory_gencalls;
		vector<pair<string,string> > auto_activated_factories;
		
		JParameterManager *jparms;
		vector<JGeometry*> geometries;
		pthread_mutex_t geometry_mutex;
		vector<JCalibration*> calibrations;
		pthread_mutex_t calibration_mutex;
		
		list<JEvent*> event_buffer;
		bool event_buffer_filling;
		pthread_mutex_t event_buffer_mutex;
		pthread_cond_t event_buffer_cond;

		vector<string> pluginPaths;
		vector<string> plugins;
		vector<JEventSourceGenerator*> eventSourceGenerators;
		vector<JFactoryGenerator*> factoryGenerators;
		vector<JCalibrationGenerator*> calibrationGenerators;
		vector<void*> sohandles;

		vector<string> args;	///< Argument list passed in to JApplication Constructor
		int show_ticker;
		int NEvents_read;		///< Number of events read from source
		int NEvents;			///< Number of events processed
		int last_NEvents;		///< Number of events processed the last time we calculated rates
		int avg_NEvents;
		double avg_time;
		double rate_instantaneous;
		double rate_average;
		vector<pthread_t> threads;
		int Ncores;				///< Number of processors currently online (sysconf(_SC_NPROCESSORS_ONLN))
		bool print_factory_report;
		bool print_resource_report;
		bool stop_event_buffer;
		bool dump_calibrations;
		bool dump_configurations;
};

} // Close JANA namespace


// The following is here just so we can use ROOT's THtml class to generate documentation.
#ifndef __CINT__

// For plugins
typedef void InitPlugin_t(jana::JApplication* app);
typedef void FiniPlugin_t(jana::JApplication* app);


// This routine is used to bootstrap plugins. It is done outside
// of the JApplication class to ensure it sees the global variables
// that the rest of the plugin's InitPlugin routine sees.
inline void InitJANAPlugin(jana::JApplication *app)
{
	// Make sure global parameter manager pointer
	// is pointing to the one being used by app
	gPARMS = app->GetJParameterManager();
}

#endif //__CINT__


#endif // _JApplication_

