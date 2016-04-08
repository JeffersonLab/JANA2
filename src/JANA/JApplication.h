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
#include <stdint.h>
#include <sysexits.h>
using std::vector;
using std::list;
using std::string;
using std::stringstream;

#include <JANA/jerror.h>
#include <JANA/JParameter.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JCalibrationGenerator.h>
#include <JANA/JEventLoop.h>
#include <JANA/JResourceManager.h>

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"


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
class JFactory_base;

typedef void CallBack_t(void *arg);

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
		                      jerror_t AddProcessor(JEventProcessor *processor, bool delete_me=false); ///< Add a JEventProcessor.
		                      jerror_t RemoveProcessor(JEventProcessor *processor); ///< Remove a JEventProcessor
		                      jerror_t AddJEventLoop(JEventLoop *loop); ///< Add a JEventLoop
		                      jerror_t RemoveJEventLoop(JEventLoop *loop); ///< Remove a JEventLoop
		                      jerror_t AddEventSource(string src_name, bool add_to_front=false); ///< Add an event source (e.g. filename) to list to be processed
		                      jerror_t AddEventSourceGenerator(JEventSourceGenerator*); ///< Add a JEventSourceGenerator
		                      jerror_t RemoveEventSourceGenerator(JEventSourceGenerator*); ///< Remove a JEventSourceGenerator
		                      jerror_t AddFactoryGenerator(JFactoryGenerator*); ///< Add a JFactory Generator
		                      jerror_t RemoveFactoryGenerator(JFactoryGenerator*); ///< Remove a JFactoryGenerator
		                      jerror_t AddCalibrationGenerator(JCalibrationGenerator*); ///< Add a JCalibrationGenerator
		                      jerror_t RemoveCalibrationGenerator(JCalibrationGenerator*); ///< Remove a JCalibrationGenerator
		      vector<JEventProcessor*> GetProcessors(void){return processors;} ///< Get the current list of JFactoryGenerators
		           vector<JEventLoop*> GetJEventLoops(void); ///< Get the current list of JEventLoops
		vector<JEventSourceGenerator*> GetEventSourceGenerators(void){return eventSourceGenerators;} ///< Get the current list of JEventSourceGenerators
                 vector<JEventSource*> GetJEventSources(void); ///< Get pointers to all JEventSource objects.
                  template<class T> T* GetFirstJEventSource(void); ///< Return pointer to first source of specified type or NULL if none exist. Call like this: ptr = GetFirstJEventSource<JEventSourceMyType>();
		                          void GetActiveEventSourceNames(vector<string> &classNames, vector<string> &sourceNames);
		    vector<JFactoryGenerator*> GetFactoryGenerators(void){return factoryGenerators;} ///< Get the current list of JFactoryGenerators
		vector<JCalibrationGenerator*> GetCalibrationGenerators(void){return calibrationGenerators;} ///< Get the current list of JCalibrationGenerators
		            JParameterManager* GetJParameterManager(void){return jparms;}
		                    JGeometry* GetJGeometry(unsigned int run_number); ///< Get the JGeometry object for the specified run number.
		                 JCalibration* GetJCalibration(unsigned int run_number); ///< Get the JCalibration object for the specified run number.
		                          void GetJCalibrations(vector<JCalibration*> &calibs){calibs=calibrations;} ///< Get the list of existing JCalibration objects
		             JResourceManager* GetJResourceManager(unsigned int run_number=0); ///< Get the JResourceManager object for the given run number (or any resource manager if no run number given)
		                      jerror_t RegisterSharedObject(const char *soname, bool verbose=true); ///< Register a dynamically linked shared object
		                      jerror_t RegisterSharedObjectDirectory(string sodirname); ///< Register all shared objects in a directory
		                      jerror_t AddPluginPath(string path); ///< Add a directory to the plugin search path
		                      jerror_t AddPlugin(const char *name); ///< Add the specified plugin to the shared objects list.
		                          void AddFactoriesToDeleteList(vector<JFactory_base*> &factories);
		              virtual jerror_t Init(void); ///< Initialize the JApplication object
		              virtual jerror_t Run(JEventProcessor *proc=NULL, int Nthreads=0); ///< Process all events from all sources
		              virtual jerror_t Fini(bool check_fini_called_flag=true); ///< Gracefully end event processing
		                  virtual void Pause(void); ///< Pause event processing
		                  virtual void Resume(void); ///< Resume event processing
		                  virtual void Quit(void); ///< Stop event processing
		                  virtual void Quit(int exit_code); ///< Stop event processing and set exit code
		                          bool GetQuittingStatus(void){return quitting;} ///< return true if Quit has already been called
		                           int GetNcores(void){return Ncores;}
					   inline uint64_t GetNEvents(void){return NEvents;} ///< Returns the number of events processed so far.
				       inline uint64_t GetNLostEvents(void){return Nlost_events;} ///< Returns the number of events processed so far.
		                  inline float GetRate(void){return rate_instantaneous;} ///< Get the average event processing rate
		                  inline float GetIntegratedRate(void){return rate_average;} ///< Get the current event processing rate
		                          void GetInstantaneousThreadRates(map<pthread_t,double> &rates_by_thread);
		                          void GetIntegratedThreadRates(map<pthread_t,double> &rates_by_thread);
		                          void GetThreadNevents(map<pthread_t,unsigned int> &Nevents_by_thread);
		                     pthread_t GetThreadID(unsigned int index); ///< Given the thread index (e.g., 0, 1, 2, ...) return the value of pthread_t for it. If outside the range, 0x0 is returned
		           const vector<void*> GetSharedObjectHandles(void){return sohandles;} ///< Get pointers to dynamically linked objects
		  vector<pair<string,string> > GetAutoActivatedFactories(void){return auto_activated_factories;}
		                          void AddAutoActivatedFactory(string name, string tag){auto_activated_factories.push_back(pair<string,string>(name,tag));}
		                  virtual void PrintRate(); ///< Print the current rate to stdout
		                          void SetShowTicker(int what){show_ticker = what;} ///< Turn auto-printing of rate to screen on or off.
		                          void SignalThreads(int signo); ///< Send a system signal to all processing threads.
		                          bool KillThread(pthread_t thr, bool verbose=true); ///< Kill a specific thread. Returns true if thread is found and kill signal sent, false otherwise.
		                  unsigned int GetNthreads(void){return threads.size();} ///< Get the current number of processing threads
		                          void SetNthreads(int new_Nthreads); ///< Set the number of processing threads to use (can be called during event processing)
		                   inline void Lock(void){WriteLock("app");} ///< Deprecated. Use ReadLock("app") or WriteLock("app") instead. (This just calls WriteLock("app").)
		                   inline void SetSequentialEventComplete(void){sequential_event_complete=true;} ///< Used by JEventLoop::Loop to signal the completion of a barrier event
		      inline pthread_rwlock_t* CreateLock(const string &name, bool throw_exception_if_exists=true);
            inline pthread_rwlock_t* ReadLock(const string &name);
            inline pthread_rwlock_t* WriteLock(const string &name);
            inline pthread_rwlock_t* Unlock(const string &name=string("app"));
            inline pthread_rwlock_t* RootReadLock(void){pthread_rwlock_rdlock(root_rw_lock); return root_rw_lock;}
            inline pthread_rwlock_t* RootWriteLock(void){pthread_rwlock_wrlock(root_rw_lock); return root_rw_lock;}
            inline pthread_rwlock_t* RootUnLock(void){pthread_rwlock_unlock(root_rw_lock); return root_rw_lock;}
            inline pthread_rwlock_t* RootFillLock(JEventProcessor *proc);
            inline pthread_rwlock_t* RootFillUnLock(JEventProcessor *proc);
                                void RegisterHUPMutex(pthread_mutex_t *mutex);
                                void RegisterHUPCallback(CallBack_t *callback, void *arg);
			  vector<pthread_mutex_t*>& GetHUPMutexes(void){ return HUP_locks; }  ///< Get list of mutexes that should be unlocked upon receiving a SIGHUP
  vector<pair<CallBack_t*, void*> >& GetHUPCallbacks(void){return HUP_callbacks;}  ///<< Get list of callback routines that should be called upon receiving a SIGHUP

                                void SetStatusBitDescription(uint32_t bit, string description); ///< Set the description of a bit in the status word used in JEvent objects
                              string GetStatusBitDescription(uint32_t bit); ///< Get the description of a bit in the status word used in JEvent objects
                                void GetStatusBitDescriptions(map<uint32_t, string> &status_bit_descriptions); ///< Get the list of all descriptions of a bit in the status word used in JEvent objects
                              string StatusWordToString(uint64_t status);
                                 int SetExitCode(int ecode){int t=ecode; exit_code=ecode; return t;}
                                 int GetExitCode(void){return exit_code;}


		bool monitor_heartbeat; ///< Turn monitoring of processing threads on/off.
		bool batch_mode;
		bool create_event_buffer_thread; ///< Set to false before Init is called to prevent the event buffer thread from being created. (Useful if program will not be reading events e.g. jcalibread.)
		int  MAX_RELAUNCH_THREADS;

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
		vector<string> source_names;
		vector<JEventSource*> sources;
		JEventSource *current_source;
		pthread_mutex_t sources_mutex;
		unsigned int Nsources_deleted;
	
		vector<JEventProcessor*> processors;
		vector<JFactory_base*> factories_to_delete;
		pthread_mutex_t factories_to_delete_mutex;
		map<pthread_t, map<string, unsigned int> > Nfactory_calls;
		map<pthread_t, map<string, unsigned int> > Nfactory_gencalls;
		vector<pair<string,string> > auto_activated_factories;
		
		JParameterManager *jparms;
		vector<JGeometry*> geometries;
		pthread_mutex_t geometry_mutex;
		vector<JCalibration*> calibrations;
		pthread_mutex_t calibration_mutex;

		vector<JResourceManager*> resource_managers;
		pthread_mutex_t resource_manager_mutex;

		list<JEvent*> event_buffer;
		bool event_buffer_filling;
		pthread_t ebthr;
		pthread_mutex_t event_buffer_mutex;
		pthread_cond_t event_buffer_cond;

		vector<string> pluginPaths;
		vector<string> plugins;
		vector<JEventSourceGenerator*> eventSourceGenerators;
		vector<JFactoryGenerator*> factoryGenerators;
		vector<JCalibrationGenerator*> calibrationGenerators;
		vector<void*> sohandles;
	
		map<string, pthread_rwlock_t*> rw_locks;
		pthread_rwlock_t rw_locks_lock; // control access to rw_locks
		pthread_rwlock_t *app_rw_lock;
		pthread_rwlock_t *root_rw_lock;
		map<JEventProcessor*, pthread_rwlock_t*> root_fill_rw_lock;
		vector<pthread_mutex_t*> HUP_locks;
		vector<pair<CallBack_t*, void*> > HUP_callbacks;

		vector<string> args;	///< Argument list passed in to JApplication Constructor
		int show_ticker;
		uint64_t NEvents_read;		///< Number of events read from source
		uint64_t NEvents;			///< Number of events processed
		uint64_t Nlost_events;		///< Number of events lost (e.g. due to stalled threads)
		uint64_t last_NEvents;		///< Number of events processed the last time we calculated rates
		uint64_t avg_NEvents;
		double avg_time;
		double rate_instantaneous;
		double rate_average;
		pthread_mutex_t threads_mutex;
		vector<JThread*> threads;
		vector<JThread*> threads_to_be_joined; // list of threads that are finished and should be joined
		int Ncores;				///< Number of processors currently online (sysconf(_SC_NPROCESSORS_ONLN))
		int Nthreads;			///< Number of desired processing threads. This can be changed during event processing via SetNtheads(N)
		bool print_factory_report;
		bool print_resource_report;
		bool stop_event_buffer;
		bool dump_calibrations;
		bool dump_configurations;
		bool list_configurations;
		bool quitting;
		bool override_runnumber;
		int  user_supplied_runnumber;
		bool sequential_event_complete;  ///< Used to flag that processing of a barrier event is complete

		int exit_code;

		map<uint32_t, string> status_bit_descriptions; ///< Descriptions of bits in status word used in JEvent
};

	
//---------------------------------
// CreateLock
//---------------------------------
inline pthread_rwlock_t* JApplication::CreateLock(const string &name, bool throw_exception_if_exists)
{
	// Lock the rw locks lock
	pthread_rwlock_wrlock(&rw_locks_lock);
	
	// Make sure a lock with this name does not already exist
	map<string, pthread_rwlock_t*>::iterator iter = rw_locks.find(name);
	pthread_rwlock_t *lock = (iter != rw_locks.end() ? iter->second:NULL);

	if(lock != NULL){
		// Lock exists. Throw exception (if specified)
		if(throw_exception_if_exists){
			pthread_rwlock_unlock(&rw_locks_lock);
			string mess = "Trying to create JANA rw lock \""+name+"\" when it already exists!";
			throw JException(mess);
		}
	}else{
		// Lock does not exist. Create it.
		lock = new pthread_rwlock_t;
		pthread_rwlock_init(lock, NULL);
		rw_locks[name] = lock;
	}
	
	// Unlock the rw locks lock
	pthread_rwlock_unlock(&rw_locks_lock);
	
	return lock;
}
	
//---------------------------------
// ReadLock
//---------------------------------
inline pthread_rwlock_t* JApplication::ReadLock(const string &name)
{
	/// Lock a global, named, rw_lock for reading. If a lock with that
	/// name does not exist, then create one and lock it for reading.
	///
	/// This is a little tricky. Access to the map of rw locks must itself
	/// be controlled by a rw lock. This means we incure the overhead of two
	/// locks and one unlock for every call to this method. Furthermore, to
	/// keep this efficient, we want to try only read locking the map at
	/// first. If we fail to find the requested lock in the map, we must 
	/// release the map's read lock and try creating the new lock.
	
	// Ensure the rw_locks map is not changed while we're looking at it,
	// lock the rw_locks_lock.
	pthread_rwlock_rdlock(&rw_locks_lock);
	
	// Find the lock. If it doesn't exist, set pointer to NULL
	map<string, pthread_rwlock_t*>::iterator iter = rw_locks.find(name);
	pthread_rwlock_t *lock = (iter != rw_locks.end() ? iter->second:NULL);
	
	// Unlock the locks lock
	pthread_rwlock_unlock(&rw_locks_lock);
	
	// If the lock doesn't exist, we need to create it. Because multiple
	// threads may be trying to do this at the same time, one may create 
	// it while another waits for the locks lock. We flag the CreateLock
	// method to not throw an exception to accommodate this.
	if(lock==NULL) lock = CreateLock(name, false);
	
	// Finally, lock the named lock or print error message if not found
	if(lock != NULL){
		pthread_rwlock_rdlock(lock);
	}else{
		string mess = "Unable to find or create lock \""+name+"\" for reading!";
		throw JException(mess);
	}
	
	return lock;
}

//---------------------------------
// WriteLock
//---------------------------------
inline pthread_rwlock_t* JApplication::WriteLock(const string &name)
{
	/// Lock a global, named, rw_lock for writing. If a lock with that
	/// name does not exist, then create one and lock it for writing.
	///
	/// This is a little tricky. Access to the map of rw locks must itself
	/// be controlled by a rw lock. This means we incure the overhead of two
	/// locks and one unlock for every call to this method. Furthermore, to
	/// keep this efficient, we want to try only read locking the map at
	/// first. If we fail to find the requested lock in the map, we must 
	/// release the map's read lock and try creating the new lock.
	
	// Ensure the rw_locks map is not changed while we're looking at it,
	// lock the rw_locks_lock.
	pthread_rwlock_rdlock(&rw_locks_lock);
	
	// Find the lock. If it doesn't exist, set pointer to NULL
	map<string, pthread_rwlock_t*>::iterator iter = rw_locks.find(name);
	pthread_rwlock_t *lock = (iter != rw_locks.end() ? iter->second:NULL);
	
	// Unlock the locks lock
	pthread_rwlock_unlock(&rw_locks_lock);
	
	// If the lock doesn't exist, we need to create it. Because multiple
	// threads may be trying to do this at the same time, one may create 
	// it while another waits for the locks lock. We flag the CreateLock
	// method to not throw an exception to accommodate this.
	if(lock==NULL) lock = CreateLock(name, false);
		
	// Finally, lock the named lock or print error message if not found
	if(lock != NULL){
		pthread_rwlock_wrlock(lock);
	}else{
		string mess = "Unable to find or create lock \""+name+"\" for writing!";
		throw JException(mess);
	}

	return lock;
}

//---------------------------------
// Unlock
//---------------------------------
inline pthread_rwlock_t* JApplication::Unlock(const string &name)
{
	/// Unlock a global, named rw_lock
	
	// To ensure the rw_locks map is not changed while we're looking at it,
	// lock the rw_locks_lock.
	pthread_rwlock_rdlock(&rw_locks_lock);
	
	// Find the lock. If it doesn't exist, set pointer to NULL
	map<string, pthread_rwlock_t*>::iterator iter = rw_locks.find(name);
	pthread_rwlock_t *lock = (iter != rw_locks.end() ? iter->second:NULL);
	
	// Unlock the locks lock
	pthread_rwlock_unlock(&rw_locks_lock);
	
	// Finally, unlock the named lock or print error message if not found
	if(lock != NULL){
		pthread_rwlock_unlock(lock);
	}else{
		string mess = "Unable to find lock \""+name+"\" for unlocking!";
		throw JException(mess);
	}

	return lock;
}

//---------------------------------
// RootFillLock
//---------------------------------
inline pthread_rwlock_t* jana::JApplication::RootFillLock(JEventProcessor *proc)
{
	/// Use this to lock a rwlock that is used exclusively by the given
	/// JEventProcessor. This addresses the common case where many plugins
	/// are in use and all contending for the same root lock. You should
	/// only use this when filling a histogram and not for creating. Use
	/// RootWriteLock and RootUnLock for that.
	map<JEventProcessor*, pthread_rwlock_t*>::iterator iter = root_fill_rw_lock.find(proc);
	if(iter==root_fill_rw_lock.end()){
		jerr << "**** Tried calling JApplication::RootFillLock with something other than a registered JEventProcessor* !!" << std::endl;
		jerr << "**** Quitting now...." << std::endl;
		Quit();
		return NULL;
	}
	pthread_rwlock_t *lock = iter->second;
	pthread_rwlock_wrlock(lock);
	return lock;
}

//---------------------------------
// RootFillUnLock
//---------------------------------
inline pthread_rwlock_t* jana::JApplication::RootFillUnLock(JEventProcessor *proc)
{
	/// Use this to unlock a rwlock that is used exclusively by the given
	/// JEventProcessor. This addresses the common case where many plugins
	/// are in use and all contending for the same root lock. You should
	/// only use this when filling a histogram and not for creating. Use
	/// RootWriteLock and RootUnLock for that.
	map<JEventProcessor*, pthread_rwlock_t*>::iterator iter = root_fill_rw_lock.find(proc);
	if(iter==root_fill_rw_lock.end()){
		jerr << "**** Tried calling JApplication::RootFillLock with something other than a registered JEventProcessor* !!" << std::endl;
		jerr << "**** Quitting now...." << std::endl;
		Quit();
		return NULL;
	}
	pthread_rwlock_t *lock = iter->second;
	pthread_rwlock_unlock(root_fill_rw_lock[proc]);
	return lock;
}

//---------------------------------
// GetFirstJEventSource
//---------------------------------
template<class T>
T* JApplication::GetFirstJEventSource(void)
{
	/// Be EXTREMELY CAUTIOUS here. The pointer returned from this
	/// could be made invalid at any point (even before returning
	/// from this method!)

	T* ptr = NULL;
	pthread_mutex_lock(&sources_mutex);
	for(unsigned int i=0; i<sources.size(); i++){
		ptr = dynamic_cast<T*>(sources[i]);
		if(ptr != NULL) break;
	}
	pthread_mutex_unlock(&sources_mutex);

	return ptr;
}
	


} // Close JANA namespace


// The following is here just so we can use ROOT's THtml class to generate documentation.
#if !defined(__CINT__) && !defined(__CLING__)

extern jana::JApplication *japp;

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

#endif //__CINT__ __CLING__


#endif // _JApplication_

