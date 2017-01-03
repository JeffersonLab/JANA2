// $Id: JApplication.cc 1842 2006-06-22 14:05:17Z davidl $
//
//    File: JApplication.cc
// Created: Wed Jun  8 12:00:20 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
using namespace std;

#ifdef __linux__
#include <execinfo.h>
#endif

#include <cstdio>
#include <signal.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "JEventLoop.h"
#include "JApplication.h"
#include "JEventProcessor.h"
#include "JEventSource.h"
#include "JEvent.h"
#include "JGeometryXML.h"
#include "JGeometryMYSQL.h"
#include "JParameterManager.h"
#include "JCalibrationFile.h"
#include "JCalibrationGeneratorCCDB.h"
#include "JEventSourceGenerator_NULL.h"
#include "JVersion.h"
#include "JStreamLog.h"
using namespace jana;

#ifndef ansi_escape
#define ansi_escape			((char)0x1b)
#define ansi_bold 			ansi_escape<<"[1m"
#define ansi_normal			ansi_escape<<"[0m"
#endif // ansi_escape

extern void force_links(void);

void* LaunchEventBufferThread(void* arg);
void* LaunchThread(void* arg);
void  CleanupThread(void* arg);

jana::JApplication *japp = NULL;

int SIGINT_RECEIVED = 0;
int SIGUSR1_RECEIVED = 0;
int NTHREADS_COMMAND_LINE = 0;

//-----------------------------------------------------------------
// ctrlCHandle
//-----------------------------------------------------------------
void ctrlCHandle(int x)
{
	SIGINT_RECEIVED++;
	jerr<<endl<<"SIGINT received ("<<SIGINT_RECEIVED<<")....."<<endl;
	if(SIGINT_RECEIVED == 3){
		jerr<<endl<<"Three SIGINTS received! Attempting graceful exit ..."<<endl<<endl;
	}
	if(SIGINT_RECEIVED ==6){
		jerr<<endl<<"Six SIGINTS received! Attempting forceful exit ..."<<endl<<endl;
		if(japp){
			japp->Fini(false);
			delete japp;
		}
		exit(-1);
	}
	if(SIGINT_RECEIVED ==9){
		jerr<<endl<<"Nine SIGINTS received! OK, I get it! ..."<<endl<<endl;
		exit(-2);
	}
}

//-----------------------------------------------------------------
// USR1_Handle
//-----------------------------------------------------------------
void USR1_Handle(int x)
{
	// This handles when the program receives a USR1(=30) signal.
	// This will dispatch the signal to all processing threads,
	// by sending a SIGUSR2 signal to them
	// causing a JException to be thrown which will be caught
	// and re-thrown while recording the factory stack info which is
	// eventually printed out.
	SIGUSR1_RECEIVED++;
	jerr<<endl<<"SIGUSR1 received ("<<SIGUSR1_RECEIVED<<").....(thread=0x"<<hex<<pthread_self()<<dec<<")"<<endl;
	if(japp)japp->SignalThreads(SIGUSR2);
}

//-----------------------------------------------------------------
// USR2_Handle
//-----------------------------------------------------------------
void USR2_Handle(int x)
{
	// This handles when the program receives a USR2(=31) signal.
	// This will dispatch the signal to all processing threads,
	// Causing a JException to be thrown which will be caught
	// and re-thrown while recording the factory stack info which is
	// eventually printed out.
	japp->Lock();
	jerr<<endl<<"SIGUSR2 received .....(thread=0x"<<hex<<pthread_self()<<dec<<")"<<endl;

#ifdef __linux__
	void * array[50];
	int nSize = backtrace(array, 50);
	char ** symbols = backtrace_symbols(array, nSize);

	jout<<endl;
	jout<<"--- Stack trace for thread=0x"<<hex<<pthread_self()<<dec<<"): ---"<<endl;
	string cmd("c++filt ");
	for (int i = 0; i < nSize; i++){
		char *ptr = strstr(symbols[i], "(");
		if(!ptr)continue;
		char symbol[255];
		strcpy(symbol, ++ptr);
		ptr = strstr(symbol,"+");
		if(!ptr)continue;
		*ptr =0;		
		cmd += symbol;
		cmd += " ";
	}
	if(system(cmd.c_str()))jerr<<"Error executing \""<<cmd<<"\""<<endl;
	jout<<endl;
#else
	jerr<<"Stack trace only supported on Linux at this time"<<endl;
#endif
	japp->Unlock();
	exit(0);
}

//-----------------------------------------------------------------
// AlarmHandle
//-----------------------------------------------------------------
void AlarmHandle(int x)
{
	jout << endl;
	jout << endl;
	jout << "----- Alarm signal caught! -----" << endl;
	jout << "This can happen if one of the hi-res timers runs out due to this" <<endl;
	jout << "being an especially long job. Some JANA accounting numbers may" << endl;
	jout << "be thrown off by this, but otherwise, it should be harmless." << endl;
	jout << endl;
	jout << endl;
}

//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(int narg, char* argv[])
{
	/// JApplication constructor
	///
	/// The arguments passed here are typically those passed into the program
	/// through <i>main()</i>. The argument list is parsed and any arguments
	/// not relevant for JANA are quietly ignored. This is the
	/// only constructor available for JApplication.
	
	// Default exit code
	SetExitCode(EX_OK); // EX_OK defined in sysexits.h
	
	// Force linking of certain routines in all executables
	if(narg<0)force_links();

	// Set up to catch SIGINTs for graceful exits
	signal(SIGINT,ctrlCHandle);

	// Set up to catch SIGUSR1s for stack dumps
	struct sigaction sigaction_usr1;
	sigaction_usr1.sa_handler = USR1_Handle;
	sigemptyset(&sigaction_usr1.sa_mask);
	sigaction_usr1.sa_flags = 0;
	sigaction(SIGUSR1, &sigaction_usr1, NULL);

	// Set up to catch SIGUSR2s for stack dumps
	struct sigaction sigaction_usr2;
	sigaction_usr2.sa_handler = USR2_Handle;
	sigemptyset(&sigaction_usr2.sa_mask);
	sigaction_usr2.sa_flags = 0;
	sigaction(SIGUSR2, &sigaction_usr2, NULL);

	// Initialize application level mutexes
	pthread_rwlock_init(&rw_locks_lock, NULL);
	pthread_mutex_init(&sources_mutex, NULL);
	pthread_mutex_init(&factories_to_delete_mutex, NULL);
	pthread_mutex_init(&geometry_mutex, NULL);
	pthread_mutex_init(&calibration_mutex, NULL);
	pthread_mutex_init(&resource_manager_mutex, NULL);
	pthread_mutex_init(&threads_mutex, NULL);
	pthread_mutex_init(&event_buffer_mutex, NULL);
	pthread_cond_init(&event_buffer_cond, NULL);
	app_rw_lock = CreateLock("app");
	root_rw_lock = CreateLock("root");
	CreateLock("status_bit_descriptions");

	// Variables used for calculating the rate
	show_ticker = 1;
	NEvents_read = 0;
	NEvents = 0;
	Nlost_events = 0;
	last_NEvents = 0;
	avg_NEvents = 0;
	avg_time = 0.0;
	rate_instantaneous = 0.0;
	rate_average = 0.0;
	monitor_heartbeat= true;
	batch_mode=false;
	create_event_buffer_thread = true;
	init_called = false;
	fini_called = false;
	stop_event_buffer = false;
	dump_calibrations = false;
	dump_configurations = false;
	list_configurations = false;
	quitting = false;
	override_runnumber = false;
	user_supplied_runnumber = 0;
	Ncores = sysconf(_SC_NPROCESSORS_ONLN);
	Nsources_deleted = 0;
	MAX_RELAUNCH_THREADS=0;

	// Configuration Parameter manager
	jparms = new JParameterManager();
	
	// Event buffer
	event_buffer_filling = true;
	
	print_factory_report = false;
	print_resource_report = false;

	
	// Loop over arguments
	current_source = NULL;
	if(narg>0)this->args.push_back(string(argv[0]));
	for(int i=1; i<narg; i++){
	
		// Record arguments
		this->args.push_back(string(argv[i]));
	
		const char *arg="--nthreads=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			NTHREADS_COMMAND_LINE = atoi(&argv[i][strlen(arg)]);
			continue;
		}
		arg="--plugin=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			const char* pluginname = &argv[i][strlen(arg)];
			AddPlugin(pluginname);
			continue;
		}
		arg="--so=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			const char* soname = &argv[i][strlen(arg)];
			RegisterSharedObject(soname);
			continue;
		}
		arg="--sodir=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			const char* sodirname = &argv[i][strlen(arg)];
			RegisterSharedObjectDirectory(sodirname);
			continue;
		}
		arg="--config=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			string fname(&argv[i][strlen(arg)]);
			jparms->ReadConfigFile(fname);
			continue;
		}
		arg="--factoryreport";
		if(!strncmp(arg, argv[i],strlen(arg))){
			print_factory_report = true;
			continue;
		}
		arg="--dumpcalibrations";
		if(!strncmp(arg, argv[i],strlen(arg))){
			dump_calibrations = true;
			continue;
		}
		arg="--dumpconfig";
		if(!strncmp(arg, argv[i],strlen(arg))){
			dump_configurations = true;
			continue;
		}
		arg="--listconfig";
		if(!strncmp(arg, argv[i],strlen(arg))){
			list_configurations = true;
			continue;
		}
		arg="--resourcereport";
		if(!strncmp(arg, argv[i],strlen(arg))){
			print_resource_report = true;
			continue;
		}
		arg="--auto_activate=";
		if(!strncmp(arg, argv[i],strlen(arg))){
			string nametag(&argv[i][strlen(arg)]);
			string name = nametag;
			string tag = "";
			string::size_type pos = nametag.find(":");
			if(pos!=string::npos){
				name = nametag.substr(0,pos);
				tag = nametag.substr(pos+1,nametag.size()-pos);
			}
			AddAutoActivatedFactory(name, tag);
			continue;
		}
		arg="-P";
		if(!strncmp(arg, argv[i],strlen(arg))){
			char* pstr = strdup(&argv[i][strlen(arg)]);
			if(!strcmp(pstr, "print")){
				// Special option "-Pprint" flags even default parameters to be printed
				jparms->SetParameter("print", "all");
				continue;
			}
			char *ptr = strstr(pstr, "=");
			if(ptr){
				*ptr = 0;
				ptr++;
				jparms->SetParameter(pstr, ptr);
			}else{
				_DBG_<<" bad parameter argument ("<<argv[i]<<") should be of form -Pkey=value"<<endl;
			}
			free(pstr);
			continue;
		}
		arg="--janaversion";
		if(!strncmp(arg, argv[i],strlen(arg))){
			cout<<"          JANA version: "<<JVersion::GetVersion()<<endl;
			cout<<"        JANA ID string: "<<JVersion::GetIDstring()<<endl;
			cout<<"     JANA SVN revision: "<<JVersion::GetSVNrevision()<<endl;
			cout<<"JANA last changed date: "<<JVersion::GetDate()<<endl;
			cout<<"              JANA URL: "<<JVersion::GetURL()<<endl;
			continue;
		}
		if(argv[i][0] == '-')continue;
		source_names.push_back(argv[i]);
	}
	
#if HAVE_CCDB
	// Optionally install a CCDB calibration generator
	AddCalibrationGenerator(new JCalibrationGeneratorCCDB());
#endif
	
	// Configure output streams based on config parameters
	string jout_tag = jout.GetTag();
	string jerr_tag = jerr.GetTag();
	bool jout_timestamp_flag = jout.GetTimestampFlag();
	bool jerr_timestamp_flag = jerr.GetTimestampFlag();
	bool jout_threadstamp_flag = jout.GetThreadstampFlag();
	bool jerr_threadstamp_flag = jerr.GetThreadstampFlag();
	jparms->SetDefaultParameter("JANA:JOUT_TAG", jout_tag, "string prefixed to all lines sent to jout ofstream");
	jparms->SetDefaultParameter("JANA:JERR_TAG", jerr_tag, "string prefixed to all lines sent to jerr ofstream");
	jparms->SetDefaultParameter("JANA:JOUT_TIMESTAMP_FLAG", jout_timestamp_flag, "if non-zero, prepend timestamp to each message printed to jout");
	jparms->SetDefaultParameter("JANA:JERR_TIMESTAMP_FLAG", jerr_timestamp_flag, "if non-zero, prepend timestamp to each message printed to jerr");
	jparms->SetDefaultParameter("JANA:JOUT_THREADSTAMP_FLAG", jout_threadstamp_flag, "if non-zero, prepend pthread id to each message printed to jout");
	jparms->SetDefaultParameter("JANA:JERR_THREADSTAMP_FLAG", jerr_threadstamp_flag, "if non-zero, prepend pthread id to each message printed to jout");
	jout.SetTag(jout_tag);
	jerr.SetTag(jerr_tag);
	jout.SetTimestampFlag(jout_timestamp_flag);
	jerr.SetTimestampFlag(jerr_timestamp_flag);
	jout.SetThreadstampFlag(jout_threadstamp_flag);
	jerr.SetThreadstampFlag(jerr_threadstamp_flag);
	
	// Check if user specified a run number via config. parameter
	try{
		jparms->GetParameter("RUNNUMBER", user_supplied_runnumber);
		override_runnumber = true; // only get here if RUNNUMBER parameter exists
	}catch(...){}
	
	// Start high resolution timers (if not already started)
	struct itimerval start_tmr;
	getitimer(ITIMER_REAL, &start_tmr);	
	if(start_tmr.it_value.tv_sec==0 && start_tmr.it_value.tv_usec==0){
		struct itimerval value, ovalue;
		value.it_interval.tv_sec = 1000000;
		value.it_interval.tv_usec = 0;
		value.it_value.tv_sec = 1000000;
		value.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &value, &ovalue);
	}
	getitimer(ITIMER_VIRTUAL, &start_tmr);	
	if(start_tmr.it_value.tv_sec==0 && start_tmr.it_value.tv_usec==0){
		struct itimerval value, ovalue;
		value.it_interval.tv_sec = 1000000;
		value.it_interval.tv_usec = 0;
		value.it_value.tv_sec = 1000000;
		value.it_value.tv_usec = 0;
		setitimer(ITIMER_VIRTUAL, &value, &ovalue);
	}
	getitimer(ITIMER_PROF, &start_tmr);	
	if(start_tmr.it_value.tv_sec==0 && start_tmr.it_value.tv_usec==0){
		struct itimerval value, ovalue;
		value.it_interval.tv_sec = 1000000;
		value.it_interval.tv_usec = 0;
		value.it_value.tv_sec = 1000000;
		value.it_value.tv_usec = 0;
		setitimer(ITIMER_PROF, &value, &ovalue);
	}
	
	// Install a handler for SIGALRM so if a timer runs out it won't kill the program
	signal(SIGALRM, AlarmHandle);
	signal(SIGPROF, SIG_IGN);
	signal(SIGVTALRM, SIG_IGN);
	
	// Global variable
	japp = this;
}

//---------------------------------
// Usage
//---------------------------------
void JApplication::Usage(void)
{
	/// Print lines identifying the default JANA command line arguments
	
	cout<<"  --janaversion            Print JANA verson information"<<endl;
	cout<<"  --nthreads=X             Launch X processing threads"<<endl;
	cout<<"  --plugin=plugin_name     Attach the plug-in named \"plugin_name\""<<endl;
	cout<<"  --so=shared_obj          Attach a plug-in with filename \"shared_obj\""<<endl;
	cout<<"  --sodir=shared_dir       Add the directory \"shared_dir\" to search list"<<endl;
	cout<<"  --config=filename        Read in the specified JANA configuration file"<<endl;
	cout<<"  --factoryreport          Dump a short report on factories at end of job"<<endl;
	cout<<"  --dumpcalibrations       Dump calibrations used in a directory at end of job"<<endl;
	cout<<"  --dumpconfig             Dump all config. parameters into file at end of job"<<endl;
	cout<<"  --listconfig             Print all config. parameters to screen and exit"<<endl;
	cout<<"  --resourcereport         Dump report on system resources used at end of job"<<endl;
	cout<<"  --auto_activate=factory  Auto activate \"factory\" for every event"<<endl;
	cout<<"  -Pkey=value              Set configuration parameter \"key\" to \"value\""<<endl;
	cout<<"  -Pprint                  Print all configuration params"<<endl;
}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{
	/// JApplication destructor
	///
	/// Releases memory allocated by this JApplication object.
	/// as well as JEventSourceGenerator, JFactoryGenerators,
	/// and JCalibrationGenerators that have been added to the
	/// application.

	for(unsigned int i=0; i<geometries.size(); i++)delete geometries[i];
	geometries.clear();
	for(unsigned int i=0; i<calibrations.size(); i++)delete calibrations[i];
	calibrations.clear();
	for(unsigned int i=0; i<eventSourceGenerators.size(); i++)delete eventSourceGenerators[i];
	eventSourceGenerators.clear();
	for(unsigned int i=0; i<factoryGenerators.size(); i++)delete factoryGenerators[i];
	factoryGenerators.clear();
	for(unsigned int i=0; i<calibrationGenerators.size(); i++)delete calibrationGenerators[i];
	calibrationGenerators.clear();

	list<JEvent*>::iterator iter;
	for(iter=event_buffer.begin(); iter!=event_buffer.end(); iter++)delete *iter;
	event_buffer.clear();

	map<string, pthread_rwlock_t*>::iterator irw_locks;
	for(irw_locks=rw_locks.begin(); irw_locks!=rw_locks.end(); irw_locks++) delete irw_locks->second;
	rw_locks.clear();

	for(unsigned int i=0; i<HUP_locks.size(); i++)delete HUP_locks[i];
	HUP_locks.clear();
	
	// Delete JParameterManager
	if(jparms)delete jparms;

	// Close any open dll's
	// This must be done after everything else since seg. faults have occurred on
	// at least one system (Ubuntu13) when JParameters created by a plugin are deleted
	// after the shared object is closed.
	for(unsigned int i=0; i<sohandles.size(); i++){
		jout<<"Closing shared object handle "<<i<<" ..."<<endl; jout.flush();

		// Look for a FiniPlugin symbol and execute if found
		FiniPlugin_t *plugin = (FiniPlugin_t*)dlsym(sohandles[i], "FiniPlugin");
		if(plugin){
			jout<<"Finalizing plugin ..."<<endl;
			(*plugin)(this);
		}
			
		// Close shared object
		dlclose(sohandles[i]);
	}
	sohandles.clear();
	
	// Delete ROOT fill locks
	map<JEventProcessor*, pthread_rwlock_t*>::iterator iter_rfl;
	for(iter_rfl=root_fill_rw_lock.begin(); iter_rfl!=root_fill_rw_lock.end(); iter_rfl++){
		delete iter_rfl->second;
	}
	root_fill_rw_lock.clear();
}

//---------------------------------
// NextEvent
//---------------------------------
jerror_t JApplication::NextEvent(JEvent &event)
{
	/// Grab an event from the event buffer. If no events are
	/// there, it will wait until:
	/// A. an event shows up
	/// B. "event_buffer_filling" flag is cleared.
	/// C. the JEventLoop's quit flag is set
	
	JEvent *myevent = NULL;
	do{
		// Lock mutex and grab next event from buffer (if there's one)
		pthread_mutex_lock(&event_buffer_mutex);
		if(event_buffer.size()>0){
			myevent = event_buffer.back();
			event_buffer.pop_back();
			
			// Wake up event buffer filler thread since a slot in the buffer
			// is now open.
			pthread_cond_signal(&event_buffer_cond);
		}
		pthread_mutex_unlock(&event_buffer_mutex);
		if(event.GetJEventLoop()->GetQuit())break;
		if((myevent==NULL) && event_buffer_filling){
			// no event in buffer so sleep for a bit so CPU
			// is not completely eaten up
			usleep(100);
		}
	}while((myevent==NULL) && event_buffer_filling);

	// It is possible that we got here because the event_buffer_filling
	// flag was cleared, but only after we failed to read an event from
	// the event buffer (so myevent is NULL). If myevent is NULL, but
	// event_buffer is not empty, then try getting it again.
	if(myevent==NULL){
		pthread_mutex_lock(&event_buffer_mutex);
		if(!event_buffer.empty()){
			myevent = event_buffer.back();
			event_buffer.pop_back();
		}
		pthread_mutex_unlock(&event_buffer_mutex);
	}
	
	// If we managed to get an event, copy it to the given
	// reference and delete the JEvent object
	if(myevent){
		// It's tempting to just copy the *myevent JEvent object into
		// event directly. This will overwrite the "loop" member
		// which we don't want. Copy the other members by hand.

		// User has option of overriding run number
		int my_runnumber = myevent->GetRunNumber();
		if(override_runnumber) my_runnumber = user_supplied_runnumber;

		event.SetJEventSource(myevent->GetJEventSource());
		event.SetRunNumber(my_runnumber);
		event.SetEventNumber(myevent->GetEventNumber());
		event.SetRef(myevent->GetRef());
		event.SetID(myevent->GetID());
		event.SetStatus(myevent->GetStatus());
		event.SetSequential(myevent->GetSequential());
		NEvents++;
		delete myevent;
		return NOERROR;
	}

	return NO_MORE_EVENT_SOURCES;
}

//----------------
// LaunchEventBufferThread
//----------------
void* LaunchEventBufferThread(void* arg)
{
	/// This routine is launched in a thread and simply
	/// calls the EventBufferThread() method of the
	/// JApplication pointer passed in through arg
	JApplication *app = (JApplication*)arg;
	app->EventBufferThread();
	
	return arg;
}

//---------------------------------
// EventBufferThread
//---------------------------------
void JApplication::EventBufferThread(void)
{
	/// This method runs continuously
	/// reading in events to keep the event_buffer list filled.
	/// It exits once it has read the last event or the value of
	/// stop_event_buffer has been set to true.
	
	uint64_t EVENTS_TO_SKIP=0;
	uint64_t EVENTS_TO_KEEP=0;
	uint64_t SKIP_TO_EVENT = 0;
	uint32_t MAX_EVENTS_IN_BUFFER = 10;
	jparms->SetDefaultParameter("EVENTS_TO_SKIP", EVENTS_TO_SKIP, "Number of events that will be read in WITHOUT calling event processor(s)");
	jparms->SetDefaultParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP, "Maximum number of events for which event processors are called before ending the program");
	jparms->SetDefaultParameter("SKIP_TO_EVENT", SKIP_TO_EVENT, "Skip to event with this event number before starting event processing.");
	jparms->SetDefaultParameter("MAX_EVENTS_IN_BUFFER", MAX_EVENTS_IN_BUFFER, "Maximum number of events to keep in event buffer (set this to 1 or greater)");
	
	jerror_t err;
	JEvent *event = NULL;
	do{
		// Lock mutex
		pthread_mutex_lock(&event_buffer_mutex);
		
		// The "event" pointer actually gets created below, but waits to get
		// pushed onto the event_buffer list until now to save locking and
		// unlocking the mutex one time.
		if(event!=NULL){
			
			// Check if the source has set the sequential flag for this
			// event. If so, then we need to let the buffer drain
			// completely, then let this event through by itself, then
			// resume event reading.
			if(event->sequential){

				// wait for buffer to drain
				pthread_mutex_unlock(&event_buffer_mutex);
				while(!event_buffer.empty()){
					if(stop_event_buffer)break;
					usleep(1000);
				}
				

				// Put this event in the buffer so it can be pulled by a 
				// thread and processed.
				pthread_mutex_lock(&event_buffer_mutex);
				event_buffer.push_front(event);
				sequential_event_complete = false;
				pthread_mutex_unlock(&event_buffer_mutex);
				
				// The sequential_event_complete flag will be set to true in
				// JEventLoop::Loop once it has completed processing.
				while(!sequential_event_complete){
					if(stop_event_buffer)break;
					usleep(1000);
				}
				pthread_mutex_lock(&event_buffer_mutex);
			
			}else{

				// normal event processing
				event_buffer.push_front(event);

			}
		}
		event=NULL;
		
		// Wait until either a slot is open to read an event into,
		// or we're told to stop.
		while(event_buffer.size()>=MAX_EVENTS_IN_BUFFER){
			pthread_cond_wait(&event_buffer_cond, &event_buffer_mutex);
			if(stop_event_buffer)break;
		}
		
		// Unlock mutex
		pthread_mutex_unlock(&event_buffer_mutex);
		if(stop_event_buffer)break;

		// The only way to get to here is if there is room in the event
		// buffer for another event. Read one in and add it to the buffer
		event = new JEvent;
		err = ReadEvent(*event);
		if(err!=NOERROR){
			delete event;
			event = NULL;
		}else{
			// If the user specified that some events should be skipped,
			// then do that here, making sure to free the event first!
			if(NEvents_read<=(uint64_t)EVENTS_TO_SKIP){
				event->FreeEvent();
				delete event;
				event = NULL;
			}
			
			// If user specified a specific event to skip to, then
			// check if this is that event.
			if(SKIP_TO_EVENT!=0){
				if(event->GetEventNumber()==SKIP_TO_EVENT){
					// This is the event they're looking for!
					// Allow the event to be processed and set 
					// SKIP_TO_EVENT to zero so subsequent events
					// are processed as well.
					SKIP_TO_EVENT = 0;
				}else{
					event->FreeEvent();
					delete event;
					event = NULL;
				}
			}
		}
		
		// If the user specified a fixed number of events to keep, then 
		// check that here and end the loop once we've read them all
		// in.
		if(EVENTS_TO_KEEP>0)
			if(NEvents_read >= (uint64_t)(EVENTS_TO_SKIP+EVENTS_TO_KEEP))break;
		
	}while(err!=NO_MORE_EVENT_SOURCES);

	// Check if we have a last event that was read in but not added to the 
	// event buffer and add it now.
	if(event!=NULL)
	pthread_mutex_lock(&event_buffer_mutex);
	event_buffer.push_front(event);
	pthread_mutex_unlock(&event_buffer_mutex);
	event=NULL;

	event_buffer_filling=false;
}

//---------------------------------
// ReadEvent
//---------------------------------
jerror_t JApplication::ReadEvent(JEvent &event)
{
	/// Call the current source's GetEvent() method. If no
	/// source is open, or the current source has no more events,
	/// then open the next source and recall ourself

	// Note that this routine only gets called from the event buffer
	// thread and so does not need to use a mutex any more. 
	
	if(!current_source){
		jerror_t err = OpenNext();
		if(err != NOERROR)return err;
		return ReadEvent(event);
	}
	
	// Read next event from source. If none, then simply set current_source
	// to NULL and recall ourself
	jerror_t err;
	try{
		err = current_source->JEventSource::GetEvent(event);
	}catch(...){
		// If we get an exception, consider this source finished!
		err = NO_MORE_EVENTS_IN_SOURCE;
	}
	switch(err){
		case NO_MORE_EVENTS_IN_SOURCE:
			current_source = NULL;
			return ReadEvent(event);
			break;
		default:
			break;
	}

	// Event counter
	NEvents_read++;

	return NOERROR;
}

//---------------------------------
// GetEventBufferSize
//---------------------------------
unsigned int JApplication::GetEventBufferSize(void)
{
	/// Returns the number of events currently in the event buffer
	pthread_mutex_lock(&event_buffer_mutex);
	unsigned int Nevents_in_buffer = event_buffer.size();
	pthread_mutex_unlock(&event_buffer_mutex);

	return Nevents_in_buffer;
}

//---------------------------------
// AddProcessor
//---------------------------------
jerror_t JApplication::AddProcessor(JEventProcessor *processor, bool delete_me)
{
	/// Add a JEventProcessor object to be included when processing events.
	/// This <b>must</b> be called before invoking the Run() method since
	/// each thread's JEventLoop will copy the list of event processors when
	/// it is created. Adding JEventProcessor objects after event processing
	/// has begun will have no effect.
	///
	/// The JEventProcessor object will be used by the system, but it will not
	/// delete it. It is up to the caller to delete it after event processing has
	/// stopped (i.e. Run() has returned).
	processor->SetJApplication(this);
	processors.push_back(processor);
	processor->SetDeleteMe(delete_me);
	
	pthread_rwlock_t *lock = new pthread_rwlock_t;
	pthread_rwlock_init(lock, NULL);
	root_fill_rw_lock[processor] = lock;

	return NOERROR;
}

//---------------------------------
// RemoveProcessor
//---------------------------------
jerror_t JApplication::RemoveProcessor(JEventProcessor *processor)
{
	/// Remove a JEventProcessor object from the list kept by the JApplication object. 
	/// This <b>must</b> be called before invoking the Run() method since
	/// each thread's JEventLoop will copy the list of event processors when
	/// it is created. Removing JEventProcessor objects after event processing
	/// has begun will have no effect.
	///
	/// The JEventProcessor object will be used by the system, but it will not
	/// delete it. It is up to the caller to delete it after event processing has
	/// stopped (i.e. Run() has returned).
	vector<JEventProcessor*>::iterator iter = processors.begin();
	for(; iter!=processors.end(); iter++){
		if((*iter) == processor){
			processors.erase(iter);
			break;
		}
	}

	return NOERROR;
}

//---------------------------------
// AddJEventLoop
//---------------------------------
jerror_t JApplication::AddJEventLoop(JEventLoop *loop)
{
	/// Add a JEventLoop object and generate a complete set of factories
	/// for it by calling the GenerateFactories method for each of the
	/// JFactoryGenerator objects registered via AddFactoryGenerator().
	/// This is typically not called directly but rather, called by
	/// the JEventLoop constructor.

	// Create a new JThread object to represent this JEventLoop
	JThread *jthread = new JThread(loop);

	// Copy pointer to the jthread object into the JEventLoop
	// so it can update the heartbeat
	loop->jthread = jthread;

	// Lock application-level mutex so we can use/modify it's members
	WriteLock("app");
	threads.push_back(jthread);

	// Loop over all factory generators, creating the factories
	// for this JEventLoop.
	for(unsigned int i=0; i<factoryGenerators.size(); i++){
		factoryGenerators[i]->GenerateFactories(loop);
	}
	
	// Release mutex
	Unlock("app");
	
	return NOERROR;
}

//---------------------------------
// RemoveJEventLoop
//---------------------------------
jerror_t JApplication::RemoveJEventLoop(JEventLoop *loop)
{
	/// Remove a JEventLoop object from the application. This is typically
	/// not called directly by the user. Rather, it is called from the
	/// JEventLoop destructor.

	WriteLock("app");

	for(unsigned int i=0; i<threads.size(); i++){
		JThread *jthread = threads[i];
		if(jthread->loop == loop){
			if(print_factory_report)RecordFactoryCalls(loop);

			threads_to_be_joined.push_back(jthread);
			threads.erase(threads.begin()+i);
			break;
		}
	}

	Unlock("app");

	return NOERROR;
}

//---------------------------------
// AddEventSource
//---------------------------------
jerror_t JApplication::AddEventSource(string src_name, bool add_to_front)
{
	/// Add an event source to the list of sources to be processed. In most
	/// cases, the user will specify the source (usually an input file) on
	/// the command line. This method provides a way to add a source 
	/// programmatically. For example, a plugin that receives events via
	/// the network could specify the network source automatically so the
	/// user doesn't need to specify both the plugin and the source.
	///
	/// The optional "add_to_front" flag can be set to true to have the
	/// source prepended to the front of the list of sources. Otherwise,
	/// it will be appended to the back.
	if(add_to_front){
		source_names.insert(source_names.begin(), src_name);
	}else{
		source_names.push_back(src_name);
	}

	return NOERROR;
}

//---------------------------------
// AddEventSourceGenerator
//---------------------------------
jerror_t JApplication::AddEventSourceGenerator(JEventSourceGenerator *generator)
{
	/// Add an event source generator to the application. This is used to
	/// decide which generator to use to read in a specific source passed
	/// on the command line and then created a JEventSource object to handle
	/// it. Ownership of the JEventSourceGenerator stays with the caller.
	eventSourceGenerators.push_back(generator);

	return NOERROR;
}

//---------------------------------
// RemoveEventSourceGenerator
//---------------------------------
jerror_t JApplication::RemoveEventSourceGenerator(JEventSourceGenerator *generator)
{
	/// Remove the specified JEventSourceGenerator object from the application.
	/// This does not delete the object, just removes it from the list.
	vector<JEventSourceGenerator*>& f = eventSourceGenerators;
	vector<JEventSourceGenerator*>::iterator iter = find(f.begin(), f.end(), generator);
	if(iter != f.end())f.erase(iter);

	return NOERROR;
}

//---------------------------------
// AddFactoryGenerator
//---------------------------------
jerror_t JApplication::AddFactoryGenerator(JFactoryGenerator *generator)
{
	/// Add a JFactoryGenerator object to the application. Ownership
	/// of the object stays with the caller.
	factoryGenerators.push_back(generator);

	return NOERROR;
}

//---------------------------------
// RemoveFactoryGenerator
//---------------------------------
jerror_t JApplication::RemoveFactoryGenerator(JFactoryGenerator *generator)
{
	/// Remove the specified JFactoryGenerator object from the application.
	/// The object is not deleted.
	vector<JFactoryGenerator*>& f = factoryGenerators;
	vector<JFactoryGenerator*>::iterator iter = find(f.begin(), f.end(), generator);
	if(iter != f.end())f.erase(iter);

	return NOERROR;
}

//---------------------------------
// AddCalibrationGenerator
//---------------------------------
jerror_t JApplication::AddCalibrationGenerator(JCalibrationGenerator *generator)
{
	/// Add a calibration generator to the application. This is used to
	/// generate a JCalibration object from the URL found in the
	/// JANA_CLAIB_URL environment variable
	calibrationGenerators.push_back(generator);

	return NOERROR;
}

//---------------------------------
// RemoveCalibrationGenerator
//---------------------------------
jerror_t JApplication::RemoveCalibrationGenerator(JCalibrationGenerator *generator)
{
	/// Remove the specified JCalibrationGenerator object from the application.
	/// This does not delete the object, just removes it from the list.
	vector<JCalibrationGenerator*>& f = calibrationGenerators;
	vector<JCalibrationGenerator*>::iterator iter = find(f.begin(), f.end(), generator);
	if(iter != f.end())f.erase(iter);

	return NOERROR;
}

//---------------------------------
// GetJEventLoops
//---------------------------------
vector<JEventLoop*> JApplication::GetJEventLoops(void)
{
	/// Return STL vector of pointers to all active 
	/// JEventLoop objects associated with this JApplication
	vector<JEventLoop*> loops;
	
	pthread_mutex_lock(&threads_mutex);
	for(uint32_t i=0; i<threads.size(); i++) loops.push_back(threads[i]->loop);
	pthread_mutex_unlock(&threads_mutex);
	
	return loops;
}

//---------------------------------
// GetActiveEventSourceNames
//---------------------------------
vector<JEventSource*> JApplication::GetJEventSources(void)
{
	/// Don't use this routine! Well, unless you know exactly what you
	/// are doing. The pointers returned here reflect a snapshot of an
	/// internal list that could change at any time. Specifically, any
	/// of the pointers could be deleted, even before returning from
	/// this call!

	// Loack the mutex to at least ensure the list can't change while
	// we're copying it.
	pthread_mutex_lock(&sources_mutex);
	vector<JEventSource*> my_sources = sources;
	pthread_mutex_unlock(&sources_mutex);

	return my_sources;
}

//---------------------------------
// GetActiveEventSourceNames
//---------------------------------
void JApplication::GetActiveEventSourceNames(vector<string> &classNames, vector<string> &sourceNames)
{
	/// Get a list of active sources (ones that have not yet been deleted)
	/// and return them in the provided containers. This puts the class names
	/// and source names into separate vectors. The elements in the first vector
	/// correspond to those in the other. It is done this way since this is used
	/// by the janactl plugin which can easily attach the data in the form of
	/// multiple vectors.
	pthread_mutex_lock(&sources_mutex);

	// Add elements in reverse order so most recent source is at beginning of lists
	for(unsigned int i=0; i<sources.size(); i++){
		JEventSource *source = sources[sources.size()-i-1];
		if(source==NULL)continue;

		classNames.push_back(source->className());
		sourceNames.push_back(source->GetSourceName());
	}

	pthread_mutex_unlock(&sources_mutex);
}

//---------------------------------
// GetJGeometry
//---------------------------------
JGeometry* JApplication::GetJGeometry(unsigned int run_number)
{
	/// Return a pointer a JGeometry object that is valid for the
	/// given run number.
	///
	/// This first searches through the list of existing JGeometry
	/// objects created by this JApplication object to see if it
	/// already has the right one.If so, a pointer to it is returned.
	/// If not, a new JGeometry object is created and added to the
	/// internal list.
	/// Note that since we need to make sure the list is not modified 
	/// by one thread while being searched by another, a mutex is
	/// locked while searching the list. It is <b>NOT</b> efficient
	/// to get the JGeometry object pointer every event. Factories
	/// should get a copy in their brun() callback and keep a local
	/// copy of the pointer for use in the evnt() callback.

	// Lock mutex to keep list from being modified while we search it
	pthread_mutex_lock(&geometry_mutex);

	vector<JGeometry*>::iterator iter = geometries.begin();
	for(; iter!=geometries.end(); iter++){
		if((*iter)->GetRunMin()>(int)run_number)continue;
		if((*iter)->GetRunMax()<(int)run_number)continue;
		// Found it! Unlock mutex and return pointer
		JGeometry *g = *iter;
		pthread_mutex_unlock(&geometry_mutex);
		return g;
	}
	
	
	// JGeometry object for this run_number doesn't exist in our list.
	// Create a new one and add it to the list.
	// We need to create an object of the appropriate subclass of
	// JGeometry. This is determined by the first several characters
	// of the URL that specifies the calibration database location.
	// For now, only the JGeometryXML subclass exists so we'll
	// just make one of those and defer the heavier algorithm until
	// later.
	const char *url = getenv("JANA_GEOMETRY_URL");
	if(!url)url="file://./";
	const char *context = getenv("JANA_GEOMETRY_CONTEXT");
	if(!context)context="default";
	
	// Decide what type of JGeometry object to create and instantiate it
	string url_str = url;
	string context_str = context;
	JGeometry *g = NULL;
	if(url_str.find("xmlfile://")==0){
		g = new JGeometryXML(string(url), run_number, context);
	}else if(url_str.find("mysql:")==0){
		g = new JGeometryMYSQL(string(url), run_number, context);
	}
	if(g)geometries.push_back(g);

	// Unlock geometry mutex
	pthread_mutex_unlock(&geometry_mutex);

	return g;
}

//---------------------------------
// GetJCalibration
//---------------------------------
JCalibration* JApplication::GetJCalibration(unsigned int run_number)
{
	/// Return a pointer to the JCalibration object that is valid for the
	/// given run number.
	///
	/// This first searches through the list of existing JCalibration
	/// objects (created by this JApplication object) to see if it
	/// already has the right one.If so, a pointer to it is returned.
	/// If not, a new JCalibration object is created and added to the
	/// internal list.
	/// Note that since we need to make sure the list is not modified 
	/// by one thread while being searched by another, a mutex is
	/// locked while searching the list. It is <b>NOT</b> efficient
	/// to get or even use the JCalibration object every event. Factories
	/// should access it in their brun() callback and keep a local
	/// copy of the required constants for use in the evnt() callback.

	// url and context may be passed in either as environment variables
	// or configuration parameters. Default values are used if neither
	// is available.
	string url     = "file://./";
	string context = "default";
	if( getenv("JANA_CALIB_URL"    )!=NULL ) url     = getenv("JANA_CALIB_URL");
	if( getenv("JANA_CALIB_CONTEXT")!=NULL ) context = getenv("JANA_CALIB_CONTEXT");
	gPARMS->SetDefaultParameter("JANA_CALIB_URL",     url,     "URL used to access calibration constants");
	gPARMS->SetDefaultParameter("JANA_CALIB_CONTEXT", context, "Calibration context to pass on to concrete JCalibration derived class");

	// Lock mutex to keep list from being modified while we search it
	pthread_mutex_lock(&calibration_mutex);

	vector<JCalibration*>::iterator iter = calibrations.begin();
	for(; iter!=calibrations.end(); iter++){
		if((*iter)->GetRun()!=(int)run_number)continue;
		if((*iter)->GetURL()!=url)continue;					// These allow specialty programs to change
		if((*iter)->GetContext()!=context)continue;		// the source and still use us to instantiate
		// Found it! Unlock mutex and return pointer
		JCalibration *g = *iter;
		pthread_mutex_unlock(&calibration_mutex);
		return g;
	}
	
	// JCalibration object for this run_number doesn't exist in our list.
	// Create a new one and add it to the list.
	// We need to create an object of the appropriate subclass of
	// JCalibration. This determined by looking through the 
	// existing JCalibrationGenerator objects and finding the
	// which claims the highest probability of being able to 
	// open it based on the URL. If there are no generators
	// claiming a non-zero probability and the URL starts with
	// "file://", then a JCalibrationFile object is created
	// (i.e. we don't bother making a JCalibrationGeneratorFile
	// class and instead, handle it here.)
	
	JCalibrationGenerator* gen = NULL;
	double liklihood = 0.0;
	for(unsigned int i=0; i<calibrationGenerators.size(); i++){
		double my_liklihood = calibrationGenerators[i]->CheckOpenable(url, run_number, context);
		if(my_liklihood > liklihood){
			liklihood = my_liklihood;
			gen = calibrationGenerators[i];
		}
	}

	// Make the JCalibration object
	JCalibration *g=NULL;
	if(gen){
		g = gen->MakeJCalibration(url, run_number, context);
	}
	if(gen==NULL && (url.find("file://")==0)){
		g = new JCalibrationFile(url, run_number, context);
	}
	if(g){
		calibrations.push_back(g);
		jout<<"Created JCalibration object of type: "<<g->className()<<endl;
		jout<<"Generated via: "<< (gen==NULL ? "fallback creation of JCalibrationFile":gen->Description())<<endl;
		jout<<"Run:"<<g->GetRun()<<endl;
		jout<<"URL: "<<g->GetURL()<<endl;
		jout<<"context: "<<g->GetContext()<<endl;
	}else{
		_DBG__;
		_DBG_<<"Unable to create JCalibration object!"<<endl;
		_DBG_<<"    URL: "<<url<<endl;
		_DBG_<<"context: "<<context<<endl;
		_DBG_<<"    run: "<<run_number<<endl;
		if(gen)
			_DBG_<<"attempted to use generator: "<<gen->Description()<<endl;
		else
			_DBG_<<"no appropriate generators found. attempted JCalibrationFile"<<endl;
	}

	// Unlock calibration mutex
	pthread_mutex_unlock(&calibration_mutex);

	return g;
}

//----------------
// GetJResourceManager
//----------------
JResourceManager* JApplication::GetJResourceManager(unsigned int run_number)
{
	/// Return a pointer to the JResourceManager object for the
	/// specified run_number. If no run_number is given or a
	/// value of 0 is given, then the first element from the
	/// list of resource managers is returned. If no managers
	/// currently exist, one will be created using one of the
	/// following in order of precedence:
	/// 1. JCalibration corrseponding to given run number
	/// 2. First JCalibration object in list (used when run_number is zero)
	/// 3. No backing JCalibration object.
	///
	/// The JCalibration is used to hold the URLs of resources
	/// so when a namepath is specified, the location of the
	/// resource on the web can be obtained and the file downloaded
	/// if necessary. See documentation in the JResourceManager
	/// class for more details.

	// Handle case for when no run number is specified
	if(run_number == 0){
		pthread_mutex_lock(&resource_manager_mutex);
		if(resource_managers.empty()){
			if(calibrations.empty()){
				resource_managers.push_back(new JResourceManager(NULL));
			}else{
				resource_managers.push_back(new JResourceManager(calibrations[0]));
			}
		}
		pthread_mutex_unlock(&resource_manager_mutex);

		return resource_managers[0];
	}

	// Run number is non-zero. Use it to get a JCalibration pointer
	JCalibration *jcalib = GetJCalibration(run_number);
	for(unsigned int i=0; i<resource_managers.size(); i++){
		if(resource_managers[i]->GetJCalibration() == jcalib)return resource_managers[i];
	}

	// No resource manager exists for the JCalibration that
	// corresponds to the given run_number. Create one.
	JResourceManager *resource_manager = new JResourceManager(jcalib);
	pthread_mutex_lock(&resource_manager_mutex);
	resource_managers.push_back(resource_manager);
	pthread_mutex_unlock(&resource_manager_mutex);

	return resource_manager;
}

//----------------
// LaunchThread
//----------------
void* LaunchThread(void* arg)
{
	/// This is a global function that is used to create
	/// a new JEventLoop object which runs in its own thread.
	
	JApplication *app = (JApplication*)arg;

	if(app!=NULL && app->GetQuittingStatus())return NULL; // in case quit has already been called
	
	// For stuck threads, we may need to cancel them at an arbitrary execution
	// point so we set our cancel type to PTHREAD_CANCEL_ASYNCHRONOUS.
	int oldtype, oldstate;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

	// Create JEventLoop object. He automatically registers himself
	// with the JApplication object in his constructor. 
	JEventLoop *loop = new JEventLoop(app);
	
	// Add a cleanup routine to this thread so he can automatically
	// de-register himself when the thread exits for any reason
	pthread_cleanup_push(CleanupThread, (void*)loop);

	// Loop over events until done. Catch any jerror_t's thrown
	try{
		loop->Loop();
		time_t t = time(NULL);
		jout<<"Thread 0x"<<hex<<(unsigned long)pthread_self()<<dec<<" completed gracefully: "<< ctime(&t);
	}catch(exception &e){
		_DBG_<<ansi_bold<<" EXCEPTION caught for thread "<<pthread_self()<<" : "<<e.what()<< ansi_normal << endl;
	}

	// This will cause the JEventLoop to be destroyed (see below) which causes
	// the entire thread, (, heartbeat, etc ...) to be removed from the JApplication.
	pthread_cleanup_pop(1);
	
	// Exit the thread
	pthread_exit(arg);

	return arg;
}

//----------------
// CleanupThread
//----------------
void CleanupThread(void* arg)
{
	/// This gets called when a thread exits so that the associated JEventLoop
	/// can be deleted.
	if(arg!=NULL){
		JEventLoop *loop = (JEventLoop*)arg;
		delete loop; // This will cause the destructor to de-register the JEventLoop
	}
}

//----------------
// AddFactoriesToDeleteList
//----------------
void JApplication::AddFactoriesToDeleteList(vector<JFactory_base*> &factories)
{
	/// Add the given list of JFactory_base objects to a list
	/// to be deleted after all of the JEventProcessor::fini()
	/// methods have been called. This allows the processors to
	/// still access data in the factories after event processing
	/// has completed. Note that at that time, the JEventLoop objects
	/// have all been destroyed so the processor will have to have
	/// maintained a copy of the factory pointer on its own.
	pthread_mutex_lock(&factories_to_delete_mutex);
	factories_to_delete.insert(factories_to_delete.end(), factories.begin(), factories.end());
	pthread_mutex_unlock(&factories_to_delete_mutex);
}

//---------------------------------
// Init
//---------------------------------
jerror_t JApplication::Init(void)
{
	/// Initialize the JApplication object. This is not typically called by
	/// the user except in GUI applications where the main event loop is
	/// handled by the GUI package. For normal event processing programs
	/// (ones that call JApplication::Run()), this is called automatically
	/// from Run().

	if(init_called){
		string mess = " Attempting to call JApplication::Init() twice!\n";
		       mess+= " A JApplication object may only be initilaized once and\n";
		       mess+= " may only process data once. Use a different JApplication\n";
		       mess+= " object if trying to re-process the data.";
		jerr << mess << endl;
		throw JException(mess);
	}
	init_called = true;

	// Attach any plugins
	AttachPlugins();

	// Add auto-activated factories in a style similar to PLUGINS
	// (comma separated list).
	try{
		string autoactivate_conf;
		jparms->GetParameter("AUTOACTIVATE", autoactivate_conf);
		if(autoactivate_conf.length()>0){
			// Loop over comma separated list of factories to auto activate
			vector<string> myfactories;
			string &str = autoactivate_conf;
			unsigned int cutAt;
			while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
				if(cutAt > 0)myfactories.push_back(str.substr(0,cutAt));
				str = str.substr(cutAt+1);
			}
			if(str.length() > 0)myfactories.push_back(str);

			// Loop over list of factory strings (which could be in factory:tag
			// form) and parse the strings as needed in order to add them to
			// the auto activate list.
			for(unsigned int i=0; i<myfactories.size(); i++){
				string nametag = myfactories[i];
				string name = nametag;
				string tag = "";
				string::size_type pos = nametag.find(":");
				if(pos!=string::npos){
					name = nametag.substr(0,pos);
					tag = nametag.substr(pos+1,nametag.size()-pos);
				}
				AddAutoActivatedFactory(name, tag);
			}
		}
	}catch(...){}

	// Allow user to specify batch mode via configuration parameter
	try{
		jparms->SetDefaultParameter("JANA:BATCH_MODE", batch_mode,"Flag that, when set to TRUE, inhibits many messages from printing. (Default is FALSE)");
	}catch(...){}

	// Call init Processors (note: factories don't exist yet)
	try{
		for(unsigned int i=0;i<processors.size();i++)processors[i]->init();
	}catch(exception &e){
		jerr<<endl;
		_DBG_<<e.what()<<endl;
		exit(-1);
	}
	
	// At this point, we may have added some event processors through plugins
	// that were not present before we were called. Any JEventLoop objects
	// that exist would not have these in their list of prcessors. Refresh
	// the lists for all event loops
	for(unsigned int i=0; i<threads.size(); i++)threads[i]->loop->RefreshProcessorListFromJApplication();

	// Launch event buffer thread
	if(create_event_buffer_thread)
		pthread_create(&ebthr, NULL, LaunchEventBufferThread, this);

	return NOERROR;
}

//---------------------------------
// Run
//---------------------------------
jerror_t JApplication::Run(JEventProcessor *proc, int Nthreads)
{
	/// Begin processing events. This will launch the specified number of threads
	/// and start them processing events. This method will return only when 
	/// all events have been processed, or processing is stopped early by the
	/// user.

	// If a JEventProcessor was passed, then add it to our list first
	if(proc){
		jerror_t err = AddProcessor(proc);
		if(err)return err;
	}

	// If user specfied that all parameters be listed, then we need to set
	// it up to do this whether they specified an event source or not.
	// We also only want to process a single event and ensure that all factories
	// are activated.
	if(list_configurations){
		AddEventSourceGenerator(new JEventSourceGenerator_NULL());
		jparms->SetParameter("EVENT_SOURCE_TYPE", "JEventSourceGenerator_NULL");
		jparms->SetParameter("EVENTS_TO_KEEP", 1);
		jparms->SetParameter("NTHREADS", 1);
		jparms->SetParameter("AUTOACTIVATE", "all");
		source_names.clear();
		source_names.push_back("dummy_source");
	}

	// Call init() for JEventProcessors (factories don't exist yet)
	Init();
		
	// If no event sources were specified, then notify the user now and exit
	if(source_names.size() == 0){
		jerr<<endl;
		jerr<<" xxxxxxxxxxxx  No event sources specified!!  xxxxxxxxxxxx"<<endl;
		jerr<<endl;
		return NO_MORE_EVENT_SOURCES;
	}
	
	// Determine number of threads to launch
	stringstream ss;
	ss<<Nthreads;
	string nthreads_str = ss.str();
	jparms->SetDefaultParameter("NTHREADS", nthreads_str, "Number of event processing threads. If set to 'Ncores' then one thread will be launched for each core the system claims to have.");
	if(nthreads_str=="Ncores"){
		Nthreads = Ncores;
	}else{
		Nthreads = atoi(nthreads_str.c_str());
	}
	if(Nthreads<1){
		// If Nthreads is less than 1 then automatically set to 1
		Nthreads = 1;
	}
	if(NTHREADS_COMMAND_LINE>0){
		Nthreads = NTHREADS_COMMAND_LINE;
	}

	// Launch all threads
	jout<<"Launching threads "; jout.flush();
	usleep(100000); // give time for above message to print before messages from threads interfere.
	this->Nthreads = Nthreads;
	for(int i=0; i<Nthreads; i++){
		pthread_t thr;
		pthread_create(&thr, NULL, LaunchThread, this);
		jout<<".";jout.flush();
	}
	jout<<endl;
	
	// Get the max time for a thread to be inactive before being deleting
	double THREAD_TIMEOUT=8.0;
	double THREAD_TIMEOUT_FIRST_EVENT=30.0;
	jparms->SetDefaultParameter("THREAD_TIMEOUT", THREAD_TIMEOUT, "Max. time (in seconds) system will wait for a thread to update its heartbeat before killing it and launching a new one.");
	jparms->SetDefaultParameter("THREAD_TIMEOUT_FIRST_EVENT", THREAD_TIMEOUT_FIRST_EVENT, "Max. time (in seconds) system will wait for first event to complete before killing program.");
	double THREAD_STALL_WARN_TIMEOUT=THREAD_TIMEOUT;
	jparms->SetDefaultParameter("THREAD_STALL_WARN_TIMEOUT", THREAD_STALL_WARN_TIMEOUT, "Max. time (in seconds) before printing a stalled thread warning (first NTHREADS events are ignored).");
	jparms->SetDefaultParameter("JANA:MAX_RELAUNCH_THREADS", MAX_RELAUNCH_THREADS, "Max. number of times to relaunch a thread due to it timing out before forcing program to quit.");
	if(THREAD_TIMEOUT_FIRST_EVENT < THREAD_TIMEOUT){
		jout << " THREAD_TIMEOUT_FIRST_EVENT is set smaller than THREAD_TIMEOUT"<<endl;
		jout << " (" << THREAD_TIMEOUT_FIRST_EVENT << " < " << THREAD_TIMEOUT << "). THREAD_TIMEOUT_FIRST_EVENT will be set to " << THREAD_TIMEOUT << endl;
		THREAD_TIMEOUT_FIRST_EVENT = THREAD_TIMEOUT;
	}
	if(THREAD_STALL_WARN_TIMEOUT > THREAD_TIMEOUT){
		jout << " THREAD_STALL_WARN_TIMEOUT is set greater than THREAD_TIMEOUT"<<endl;
		jout << " (" << THREAD_STALL_WARN_TIMEOUT << " > " << THREAD_TIMEOUT << "). THREAD_STALL_WARN_TIMEOUT will be set to " << THREAD_TIMEOUT << endl;
		THREAD_STALL_WARN_TIMEOUT = THREAD_TIMEOUT;
	}
	
	// Do a sleepy loop so the threads can do their work
	struct timespec req, rem;
	req.tv_nsec = (int)0.5E9; // set to 1/2 second
	req.tv_sec = 0;
	double sleep_time = (double)req.tv_sec + (1.0E-9)*(double)req.tv_nsec;
	int Nrelaunch_threads = 0;
	int Nstalled_threads=0;
	do{
		// Sleep for a specific amount of time and calculate the rate
		// on each iteration through the loop
		rem.tv_sec = rem.tv_nsec = 0;
		nanosleep(&req, &rem);
		if(rem.tv_sec == 0 && rem.tv_nsec == 0){
			// If there was no time remaining, then we must have slept
			// the whole amount
			int delta_NEvents = NEvents - last_NEvents;
			if(NEvents>(uint64_t)this->Nthreads){
				avg_NEvents += delta_NEvents>0 ? delta_NEvents:0;
				avg_time += sleep_time;
			}
			rate_instantaneous = sleep_time>0.0 ? (double)delta_NEvents/sleep_time:0.0;
			rate_average = avg_time>0.0 ? (double)avg_NEvents/avg_time:0.0;
			
			// In case Quit was called while a thread was being launched ...
			if(quitting)Quit();
			
		}else{
			if(!batch_mode)jerr<<" didn't sleep full "<<sleep_time<<" seconds!"<<endl;
		}
		last_NEvents = NEvents; // NEvents counts only events that have been extracted from the buffer (not the ones still in the buffer)
		
		// If show_ticker is set, then update the screen with the rate(s)
		if(show_ticker && (!batch_mode) && threads.size()>0)PrintRate();
		
		if(SIGINT_RECEIVED)Quit();
		if(SIGINT_RECEIVED>=3)break;
		
		// Lock the app so we can check system health (heartbeats etc.)
		pthread_rwlock_rdlock(app_rw_lock);
		
		// Add time slept to all heartbeats
		double rem_time = (double)rem.tv_sec + (1.0E-9)*(double)rem.tv_nsec;
		double slept_time = sleep_time - rem_time;
		for(unsigned int i=0;i<threads.size();i++){
			double *hb = &threads[i]->heartbeat;
			*hb += slept_time;
			
			// Print warning if stalled for more than THREAD_STALL_WARN_TIMEOUT so
			// user knows info (run, event) on what's stalling
			if(monitor_heartbeat && ((*hb-slept_time) > THREAD_STALL_WARN_TIMEOUT)){
				if(!threads[i]->printed_stall_warning){
					uint64_t runnum = threads[i]->loop->GetJEvent().GetRunNumber();
					uint64_t eventnum = threads[i]->loop->GetJEvent().GetEventNumber();
					jerr << "thread " << i << " has stalled on run:" << runnum << " event:" << eventnum << endl;
					threads[i]->printed_stall_warning = true;
				}
			}else{
				// Reset in case we've moved on to another event so info
				// on next stalled event will be printed as well.
				threads[i]->printed_stall_warning = false;
			}

			// Choose timeout depending on whether the first event for all threads
			// has completed or not.
			double timeout = last_NEvents<=(uint64_t)this->Nthreads ? THREAD_TIMEOUT_FIRST_EVENT:THREAD_TIMEOUT;

			if(monitor_heartbeat && ((*hb-slept_time) > timeout)){
				// Thread hasn't done anything for more than timeout seconds. 
				// Remove it from monitoring lists.
				JEventLoop *loop = (*(threads.begin()+i))->loop;
				JEvent &event = loop->GetJEvent();
				jerr<<" Thread "<<i<<" ("<<hex<<threads[i]->thread_id<<dec<<") hasn't responded in "<<*hb<<" seconds.";
				jerr<<" (run:event="<<event.GetRunNumber()<<":"<<event.GetEventNumber()<<")";
				jerr<<" Cancelling ..."<<endl;
				
				// If we haven't processed one event per thread yet, then assume we
				// are stuck on the first event and so quit the whole program.
				if(last_NEvents<(uint64_t)this->Nthreads){
					jerr<<"    -- stalled in first Nthreads events. Killing all threads ..." << endl;
					Quit(); // nicely tell all threads to quit (set their "quit" flag.
					for(unsigned int j=0;j<threads.size();j++){
						pthread_kill(threads[j]->thread_id, SIGHUP); // not-so-nicely kill all threads
					}
					break; // everyone is dying now so stop checking heartbeats
				}
				
				// At this point, we need to kill the stalled thread. One would normally
				// do this by calling pthread_cancel() but that seems to have problems.
				// Namely, it was observed that when the thread was deep in a Xerces call
				// that it would die with a mutex locked causing other threads to wait
				// endlessly in pthread_mutex_lock for it to open back up. The way we do
				// this here is to send a HUP signal to the thread which interrupts
				// it immediately allowing it's signal handler to call pthread_exit
				// and thereby invoking the CleanupThread() routine. I don't actually
				// understand why we don't run into the same mutex problem this way
				// but empirically, it seems to work.   3/5/2009  DL
				pthread_kill(threads[i]->thread_id, SIGHUP);
				
				// When we kill a thread that has stalled we have to go to some special
				// effort to make sure a replacement gets relaunched. What can happen
				// is that the number of processing threads can drop to zero which
				// would cause the main do-loop here to exit. The Nstalled_threads counter
				// is also used to help decide whether to actually launch a new thread or
				// not.
				Nstalled_threads++;
				Nlost_events++; // keep track of events we lost.
			}
		}
		
		// If there are less threads running than specified and we are not trying to
		// quit, then launch new threads to get us up to the specified amount.
		for(unsigned int i=threads.size(); (int)i<this->Nthreads; i++){
			
			bool launch_new_thread = false;
			if(Nstalled_threads > 0) launch_new_thread = true;
			if(event_buffer_filling) launch_new_thread = true;
			if(SIGINT_RECEIVED || quitting) launch_new_thread = false;
			
			// Launch a new thread to take his place, but only if we're not trying to quit
			if(launch_new_thread){
			
				// Check if we've already reached the limit of the number of threads
				// that can be relaunched.
				if(Nrelaunch_threads>=MAX_RELAUNCH_THREADS){
					if(MAX_RELAUNCH_THREADS > 0){
						jerr<<" Too many thread relaunches ("<<MAX_RELAUNCH_THREADS<<") quitting ..."<<endl;
					}else{
						static bool message_shown = false;
						if(!message_shown){
							message_shown = true;
							jerr<<" "<<endl;
							jerr<<" Automatic relaunching of threads is disabled. If you wish to"<<endl;
							jerr<<" have the program relaunch a replacement thread when a stalled"<<endl;
							jerr<<" one is killed, set the JANA:MAX_RELAUNCH_THREADS configuration"<<endl;
							jerr<<" parameter to a value greater than zero. E.g.:"<<endl;
							jerr<<" "<<endl;
							jerr<<"     jana -PJANA:MAX_RELAUNCH_THREADS=10"<<endl;
							jerr<<" "<<endl;
							jerr<<" The program will quit now."<<endl;
							jerr<<" ";
						}
					}
					Nstalled_threads = 0; // Don't let stalled thread count hold us up
					Quit(EX_SOFTWARE);
					break;
				}
			
				jerr<<" Launching new thread ..."<<endl;
				pthread_t thr;
				pthread_create(&thr, NULL, LaunchThread, this);
				if(Nstalled_threads>0){
					// Only count threads launched here as "re"launched if
					// a stalled thread was killed. (We can also be launching
					// threads because a janactl message came it that told us
					// to increase the number of processing threads)
					Nrelaunch_threads++;
					Nstalled_threads--;
				}

				// We need to wait for the new thread to create a new JEventLoop
				// If we don't wait for this here, then control may fall to the
				// end of the while loop that checks threads.size() before the new
				// loop is added, causing the program to finish prematurely.
				for(int j=0; j<40; j++){
					struct timespec req, rem;
					req.tv_nsec = (int)0.100E9; // set to 100 milliseconds
					req.tv_sec = 0;
					rem.tv_sec = rem.tv_nsec = 0;
					pthread_rwlock_unlock(app_rw_lock);
					nanosleep(&req, &rem);
					pthread_rwlock_rdlock(app_rw_lock); // Re-lock the mutex
					// Our new thread will be in "threads" once it's up and running
					bool found_thread = false;
					for(uint32_t k=0; k<threads.size(); k++){
						if(threads[k]->thread_id == thr){
							found_thread = true;
							break;
						}
					}
					
					// If the new thread has launched, then stop waiting for it
					if(found_thread) break;
				}
			}
		}
		
		// If there are more threads running than specified and we are not trying to
		// quit, then kill enough threads to get us down to the specified amount.
		for(int i=this->Nthreads; i<(int)threads.size(); i++){
			
			// Make sure we're not trying to quit
			if( !SIGINT_RECEIVED && !quitting){
						
				jerr<<" Removing thread (to reduce number of threads) ..."<<endl;
				pthread_kill(threads[i]->thread_id, SIGHUP);
			}
		}

		// At this point we may need to write to the threads_to_be_joined
		// vector so we need to turn the read lock into a write lock
		pthread_rwlock_unlock(app_rw_lock);
		pthread_rwlock_wrlock(app_rw_lock);

		// Merge up threads that have already finished processing
		for(unsigned int i=0; i<threads_to_be_joined.size(); i++){
			void *ret;
			jout<<"Merging thread "<<i<<" (0x"<<hex<<threads_to_be_joined[i]->thread_id<<dec<<") ..."<<endl; jout.flush();
			pthread_join(threads_to_be_joined[i]->thread_id, &ret);
			delete threads_to_be_joined[i];
		}
		threads_to_be_joined.clear();
		
		// We're done with the threads, ... vectors for now. Unlock the mutex.
		pthread_rwlock_unlock(app_rw_lock);

		// Check for event sources that have finished so we can delete the JEventSource
		// object. These can allocate lots of memory and if the user passes a lot
		// of files on the command line, then the memory usage keeps piling up.
		pthread_mutex_lock(&sources_mutex);
		for(unsigned int i=0; i<sources.size(); i++){
			if(sources[i]==NULL) continue;
			if(sources[i]==current_source) continue;
			if(sources[i]->IsFinished()){
				delete sources[i];
				sources[i] = NULL;
				Nsources_deleted++;
			}
		}
		pthread_mutex_unlock(&sources_mutex);

		// When a JEventLoop runs out of events, it removes itself from
		// the list before returning from the thread.
		// It is possible at this point though that we have sent a SIGHUP to
		// the last thread(s) and they have quit, but not until after passing
		// the thread re-launch code above so there are now zero threads but
		// still more events to process. If a SIGHUP signal was sent this
		// iteration, then we do not exit the loop until the next iteration.
		// That will give us a chance to launch the replacement thread and
		// allow it to die naturally before we stop event processing.
	}while(threads.size() > 0 || Nstalled_threads>0);
	
	// Only be nice about exiting if the user wasn't insistent
	if(SIGINT_RECEIVED<3){	
		// Call erun() and fini() methods and delete event sources
		Fini();
		
		// Merge up threads that have already finished processing
		for(unsigned int i=0; i<threads_to_be_joined.size(); i++){
			void *ret;
			jout<<"Merging thread "<<i<<" (0x"<<hex<<threads_to_be_joined[i]->thread_id<<dec<<") ...."<<endl; jout.flush();
			pthread_join(threads_to_be_joined[i]->thread_id, &ret);
		}
		threads_to_be_joined.clear();
		
		// Merge up all the threads
		for(unsigned int i=0; i<threads.size(); i++){
			void *ret;
			jout<<"Merging thread "<<i<<" (0x"<<hex<<threads[i]->thread_id<<dec<<") ....."<<endl; jout.flush();
			pthread_join(threads[i]->thread_id, &ret);
		}
		threads.clear();
		
		// Event buffer thread
		jout<<"Merging event reader thread ..."<<endl; jout.flush();
		pthread_join(ebthr, NULL);

	}else{
		jout<<"Exiting hard due to catching 3 or more SIGINTs ..."<<endl;
	}
	
	jout<<" "<<NEvents-Nlost_events<<" events processed ";
	jout<<" ("<<NEvents_read<<" events read) ";
	jout<<"Average rate: "<<Val2StringWithPrefix(rate_average)<<"Hz"<<endl;
	if(Nrelaunch_threads > 0) jout<<" "<<Nrelaunch_threads<<" thread relaunches were required"<<endl;

	if(SIGINT_RECEIVED>=3)exit(-1);

	return NOERROR;
}

//---------------------------------
// Fini
//---------------------------------
jerror_t JApplication::Fini(bool check_fini_called_flag)
{
	/// Close out at the end of event processing. All <i>erun()</i> routines
	/// and <i>fini()</i> routines are called as necessary. All JEventSources
	/// are deleted and the final report is printed, if specified.

	// Checking of the fini_called flag is an option since we may get called
	// as a last desperate attempt to force a clean exit from the singal handler.
	// Locking the mutex in order to check the flag could stall us again if
	// the mutex is already locked.
	if(check_fini_called_flag){
		pthread_rwlock_wrlock(app_rw_lock);
		if(fini_called){pthread_rwlock_unlock(app_rw_lock); return NOERROR;}
		fini_called=true;
		pthread_rwlock_unlock(app_rw_lock);
	}

	// Print final resource report
	if(print_resource_report)PrintResourceReport();
	
	// Make sure erun is called
	for(unsigned int i=0;i<processors.size();i++){
		JEventProcessor *proc = processors[i];
		if(proc->brun_was_called() && !proc->erun_was_called()){
			try{
				proc->erun();
			}catch(exception &e){
				jerr<<endl;
				_DBG_<<e.what()<<endl;
			}
			proc->Set_erun_called();
		}
	}

	// Call fini Processors
	try{
		for(unsigned int i=0;i<processors.size();i++)processors[i]->fini();
	}catch(exception &e){
		jerr<<endl;
		_DBG_<<e.what()<<endl;
	}
	
	// Delete all processors that are marked for us to delete
	try{
		for(unsigned int i=0;i<processors.size();i++)if(processors[i]->GetDeleteMe())delete processors[i];
	}catch(...){
		jerr<<endl;
		_DBG_<<" Error thrown from JEventProcessor destructor!"<<endl;
	}
	processors.clear();

	// Delete all factories registered for delayed deletion
	pthread_mutex_lock(&factories_to_delete_mutex);
	for(unsigned int i=0; i<factories_to_delete.size(); i++){
		try{
			delete factories_to_delete[i];
		}catch(exception &e){
			jerr<<endl;
			_DBG_<<" Error thrown while deleting JFactory<";
			jerr<<factories_to_delete[i]->GetDataClassName()<<">"<<endl;
			_DBG_<<e.what()<<endl;
		}
	}
	factories_to_delete.clear();
	pthread_mutex_unlock(&factories_to_delete_mutex);
	
	// Delete all sources allowing them to close cleanly
	pthread_mutex_lock(&sources_mutex);
	for(unsigned int i=0;i<sources.size();i++){
		if(sources[i]!=NULL){
			delete sources[i];
			Nsources_deleted++;
		}
	}
	sources.clear();
	pthread_mutex_unlock(&sources_mutex);
	
	// Tell event buffer thread to quit (if he hasn't already)
	for(int i=0; i<10; i++){
		if(!event_buffer_filling)break;
		stop_event_buffer = true;
		pthread_cond_signal(&event_buffer_cond);
		usleep(100000);
	}
	
	// List configuration parameters (if requested)
	if(list_configurations) jparms->DumpSuccinct();
	
	// Dump configuration parameters (if requested)
	if(dump_configurations)jparms->WriteConfigFile("jana.config");
	
	// Dump calibrations (if requested)
	if(dump_calibrations){
		for(unsigned int i=0; i<calibrations.size(); i++)calibrations[i]->DumpCalibrationsToFiles();
	}

	// Print final factory report
	if(print_factory_report)PrintFactoryReport();
	
	return NOERROR;
}

//---------------------------------
// Pause
//---------------------------------
void JApplication::Pause(void)
{
	/// Pause all JEventLoops to stop event processing. Events currently
	/// being processed will be finished and the loops will pause at the
	/// beginning of the next event.
	vector<JThread*>::iterator iter = threads.begin();
	for(; iter!=threads.end(); iter++){
		(*iter)->loop->Pause();
	}
}

//---------------------------------
// Resume
//---------------------------------
void JApplication::Resume(void)
{
	/// Resume event processing for all threads.
	vector<JThread*>::iterator iter = threads.begin();
	for(; iter!=threads.end(); iter++){
		(*iter)->loop->Resume();
	}
}

//---------------------------------
// Quit
//---------------------------------
void JApplication::Quit(void)
{
	/// Quit the application. This will invoke the Quit() method of all
	/// JEventLoops. This does not force a runaway thread to be killed and
	/// will only cause the program to quit if all JEventLoops quit cleanly.
	quitting = true;
	jout<<endl<<"Telling all threads to quit ..."<<endl;
	vector<JThread*>::iterator iter = threads.begin();
	for(; iter!=threads.end(); iter++){
		(*iter)->loop->Quit();
	}
}

//---------------------------------
// Quit
//---------------------------------
void JApplication::Quit(int exit_code)
{
	/// Wrapper for Quit(void) that will first set the
	/// JApplication exit code value.
	SetExitCode(exit_code);
	Quit();
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
	sprintf(str,"%3.1f%s", val, units);

	return string(str);
}
	
//----------------
// PrintRate
//----------------
void JApplication::PrintRate(void)
{
	string event_str = Val2StringWithPrefix(NEvents) + " events processed";
	string event_read_str = "(" + Val2StringWithPrefix(NEvents_read) + " events read)";
	string ir_str = Val2StringWithPrefix(rate_instantaneous) + "Hz";
	string ar_str = Val2StringWithPrefix(rate_average) + "Hz";
	cout<<"  "<<event_str;
	cout<<"  "<<event_read_str;
	cout<<"  "<<ir_str;
	cout<<"  (avg.: "<<ar_str<<")";
	cout<<"     \r";
	cout.flush();
}

//---------------------------------
// OpenNext
//---------------------------------
jerror_t JApplication::OpenNext(void)
{
	/// Open the next source in the list. If there are none,
	/// then return NO_MORE_EVENT_SOURCES
	
	pthread_mutex_lock(&sources_mutex);

	if(sources.size() >= source_names.size()){
		pthread_mutex_unlock(&sources_mutex);
		return NO_MORE_EVENT_SOURCES;
	}
	
	// Check if the user has forced a specific type of event source
	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
	// search for that source and use it. Otherwise, throw an exception.
	JEventSourceGenerator* gen = NULL;
	const char *sname = source_names[sources.size()].c_str();
	if(gPARMS->Exists("EVENT_SOURCE_TYPE")){
		string EVENT_SOURCE_TYPE="";
		gPARMS->GetParameter("EVENT_SOURCE_TYPE", EVENT_SOURCE_TYPE);
		for(unsigned int i=0; i<eventSourceGenerators.size(); i++){
			if(eventSourceGenerators[i]->className() != EVENT_SOURCE_TYPE) continue;
			gen = eventSourceGenerators[i];
			jout << "Forcing use of event source type: " << EVENT_SOURCE_TYPE << endl;
			break;
		}
		
		if(!gen){
			jerr << endl;
			jerr << "-----------------------------------------------------------------" << endl;
			jerr << " You specified event source type \"" << EVENT_SOURCE_TYPE << "\"" << endl;
			jerr << " be used to read the event sources but no such type exists." << endl;
			jerr << " Here is a list of available source types:" << endl;
			jerr << endl;
			for(unsigned int i=0; i<eventSourceGenerators.size(); i++){
				jerr << "   " << eventSourceGenerators[i]->className() << endl;
			}
			jerr << endl;
			jerr << "-----------------------------------------------------------------" << endl;
			Quit(EX_UNAVAILABLE);
		}
	}else{
	
		// Loop over JEventSourceGenerator objects and find the one
		// (if any) that has the highest chance of being able to read
		// this source. The return value of 
		// JEventSourceGenerator::CheckOpenable(source) is a liklihood that
		// the named source can be read by the JEventSource objects
		// created by the generator. In most cases, the liklihood will
		// be either 0.0 or 1.0. In the case that 2 or more generators return
		// equal liklihoods, the first one in the list will be used.
		double liklihood = 0.0;
		for(unsigned int i=0; i<eventSourceGenerators.size(); i++){
			double my_liklihood = eventSourceGenerators[i]->CheckOpenable(sname);
			if(my_liklihood > liklihood){
				liklihood = my_liklihood;
				gen = eventSourceGenerators[i];
			}
		}
	}
	
	current_source = NULL;
	if(gen != NULL){
		jout<<"Opening source \""<<sname<<"\" of type: "<<gen->Description()<<endl;
		current_source = gen->MakeJEventSource(sname);
	}

	if(!current_source){
		jerr<<endl;
		jerr<<"  xxxxxxxxxxxx  Unable to open event source \""<<sname<<"\"!  xxxxxxxxxxxx"<<endl;
		jerr<<endl;
		if(sources.size()+1 == source_names.size()){
			unsigned int Nnull_sources = 0;
			for(unsigned int i=0; i<sources.size(); i++){
				if(sources[i] == NULL)Nnull_sources++;
			}
			if( (Nnull_sources==sources.size()) && (Nsources_deleted==0) ){
				jerr<<"   xxxxxxxxxxxx  NO VALID EVENT SOURCES GIVEN !!!   xxxxxxxxxxxx  "<<endl;
				jerr<<endl;
				Quit(EX_NOINPUT);
				pthread_mutex_unlock(&sources_mutex);
				return NO_MORE_EVENT_SOURCES;
			}
		}
	}
	
	// Add source to list (even if it's NULL!)
	sources.push_back(current_source);
	
	pthread_mutex_unlock(&sources_mutex);
	
	return NOERROR;
}

//---------------------------------
// RegisterSharedObject
//---------------------------------
jerror_t JApplication::RegisterSharedObject(const char *soname, bool verbose)
{
	// Open shared object
	void* handle = dlopen(soname, RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
	if(!handle){
		if(verbose)jerr<<dlerror()<<endl;
		return RESOURCE_UNAVAILABLE;
	}
	
	// Look for an InitPlugin symbol
	InitPlugin_t *plugin = (InitPlugin_t*)dlsym(handle, "InitPlugin");
	if(plugin){
		jout<<"Initializing plugin \""<<soname<<"\" ..."<<endl;
		(*plugin)(this);
		sohandles.push_back(handle);
	}else{
		dlclose(handle);
		if(verbose)jout<<" --- Nothing useful found in "<<soname<<" ---"<<endl;
	}

	return NOERROR;
}

//---------------------------------
// RegisterSharedObjectDirectory
//---------------------------------
jerror_t JApplication::RegisterSharedObjectDirectory(string sodirname)
{
	/// Attempt to register every file in the specified directory
	/// as a shared object file. If the file is not a shared object
	/// file, or does not have the InitPlugin symbol, it will be
	/// quietly ignored.
	jout<<"Looking for plugins in \""<<sodirname<<"\" ..."<<endl;
	DIR *dir = opendir(sodirname.c_str());

	struct dirent *d;
	char full_path[512];
	while((d=readdir(dir))){
		// Try registering the file as a shared object.
		if(!strncmp(d->d_name,".nfs",4)){
			jout<<"Skipping .nfs file :"<<d->d_name<<endl;
			continue;
		}
		sprintf(full_path, "%s/%s",sodirname.c_str(), d->d_name);
		RegisterSharedObject(full_path, false);
	}

	return NOERROR;
}

//---------------------------------
// AddPluginPath
//---------------------------------
jerror_t JApplication::AddPluginPath(string path)
{
	pluginPaths.push_back(path);

	return NOERROR;
}

//---------------------------------
// AddPlugin
//---------------------------------
jerror_t JApplication::AddPlugin(const char *name)
{
	/// Add the specified plugin to the shared objects list. The value
	/// of "name" should be something like "bcal_hists" or "track_hists".
	/// This will look for a file with the given name and a ".so" extension
	/// in directories listed in the pluginPaths vector. Paths are added
	/// via the AddPluginPath(string path) method.
	///
	/// Note that plugins are not actually searched for until the Init()
	/// method of JApplications is called either explicitly by the
	/// user, or implicitly through a call to Run().
	///
	/// A message is printed indicating each of the locations which the
	/// the plugin is be searched for. If the plugin is found, then
	/// a value of NOERROR is returned.
	/// If the plugin is not found, then a value of RESOURCE_UNAVAILABLE
	/// is returned.
	///
	/// The first occurance of a file with the given name is used.
	/// The directories are searched in the order in which they
	/// are were added.
	/// By default, the path list is initialized only with "."
	/// (i.e. the current working directory.) It is assumed that
	/// subclasses of JApplication will be used that customize
	/// JANA for a particular application and it is there that additional
	/// paths will be added. For instance, the Hall-D subclass
	/// DApplication adds paths based on the <i>HALLD_MY</i> and
	/// <i>HALLD_HOME</i> environment variables.
	/// 
	/// Note also that for each path starting with "/", 
	/// two locations are actually checked.
	/// the first is the directory given by the pathname. The second
	/// looks in a directory relative to the given path with
	/// the plugin name. For example, consider a plugin named <i>foo</i> 
	/// and a path of <i>/location/of/plugins</i>.
	/// The following would be searched for in this order:
	///
	/// <p><i>/location/of/plugins/foo.so</i></p>
	/// <p><i>/location/of/plugins/foo/foo.so</i></p>
	///
	/// This allows one to keep the plugins inside a directory
	/// structure with the source.
	
	plugins.push_back(string(name));
	
	return NOERROR;
}

//---------------------------------
// AttachPlugins
//---------------------------------
jerror_t JApplication::AttachPlugins(void)
{
	/// Loop over list of plugin names added via AddPlugin() and
	/// actually attach and intiailize them. See AddPlugin method
	/// for more.
	
	bool printPaths=false;
	try{
		jparms->GetParameter("PRINT_PLUGIN_PATHS", printPaths);
	}catch(...){}
	
	// In order to give priority to factories added via plugins,
	// the list of factory generators needs to be cleared so
	// those added from plugins will be at the front of the list.
	// We make a copy of the existing generators first so we can
	// append them back to the end of the list before exiting.
	// Similarly for event source generators and calibration generators.
	vector<JEventSourceGenerator*> my_eventSourceGenerators = eventSourceGenerators;
	vector<JFactoryGenerator*> my_factoryGenerators = factoryGenerators;
	vector<JCalibrationGenerator*> my_calibrationGenerators = calibrationGenerators;
	eventSourceGenerators.clear();
	factoryGenerators.clear();
	calibrationGenerators.clear();

	/// The JANA_PLUGIN_DIR environment is used to specify
	/// directories in which every file is attached as a plugin.
	/// Files that are not shared object files or do not have
	/// the InitPlugin symbol defined are ignored. Multiple
	/// directories can be specified using a colon(:) separator.
	const char *jpd = getenv("JANA_PLUGIN_DIR");
	if(jpd){
		string str(jpd);
		unsigned int cutAt;
		while( (cutAt = str.find(":")) != (unsigned int)str.npos ){
			if(cutAt > 0)RegisterSharedObjectDirectory(str.substr(0,cutAt));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)RegisterSharedObjectDirectory(str);
	}
	
	/// The JANA_PLUGIN_PATH specifies directories to search
	/// for plugins that were explicitly added through AddPlugin(...).
	/// Multiple directories can be specified using a colon(:) separator.
	const char *jpp = getenv("JANA_PLUGIN_PATH");
	if(jpp){
		string str(jpp);
		unsigned int cutAt;
		while( (cutAt = str.find(":")) != (unsigned int)str.npos ){
			if(cutAt > 0)AddPluginPath(str.substr(0,cutAt));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)AddPluginPath(str);
	}
	
	// Default plugin search path
	AddPluginPath(".");
	if(const char *ptr = getenv("JANA_HOME")){
		AddPluginPath(string(ptr) + "/plugins");
	}
	
	// Add plugins specified via PLUGINS configuration parameter
	// (comma separated list).
	string plugins_conf;
	try{
		jparms->GetParameter("PLUGINS", plugins_conf);
	}catch(...){}
	if(plugins_conf.length()>0){
		string &str = plugins_conf;
		unsigned int cutAt;
		while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
			if(cutAt > 0)plugins.push_back(str.substr(0,cutAt));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)plugins.push_back(str);
	}
	
	// Loop over plugins
	for(unsigned int j=0; j<plugins.size(); j++){
		// Sometimes, the user will include the ".so" suffix in the
		// plugin name. If they don't, then we add it here.
		string plugin = plugins[j];
		if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";
	
		// Loop over paths
		bool found_plugin=false;
		for(unsigned int i=0; i< pluginPaths.size(); i++){
			string fullpath = pluginPaths[i] + "/" + plugin;
			if(printPaths) jout<<"Looking for \""<<fullpath<<"\" ...."; cout.flush();
			ifstream f(fullpath.c_str());
			if(f.is_open()){
				f.close();
				if(printPaths) jout<<"found." << endl;
				if(RegisterSharedObject(fullpath.c_str(), printPaths)==NOERROR){
					found_plugin=true;
					break;
				}
			}
			if(printPaths) jout<<"Failed to attach \""<<fullpath<<"\""<<endl;
			
			if(fullpath[0] != '/')continue;
			fullpath = pluginPaths[i] + "/" + plugins[j] + "/" + plugin;
			if(printPaths) jout<<"Looking for \""<<fullpath<<"\" ...."; cout.flush();
			f.open(fullpath.c_str());
			if(f.is_open()){
				f.close();
				if(printPaths) jout<<"found." << endl;
				if(RegisterSharedObject(fullpath.c_str(), printPaths)==NOERROR){
					found_plugin=true;
					break;
				}
			}
			if(printPaths) jout<<"Failed to attach \""<<fullpath<<"\""<<endl;
		}
		
		// If we didn't find the plugin, then complain and quit
		if(!found_plugin){
			Lock();
			jerr<<endl<<"***ERROR : Couldn't find plugin \""<<plugins[j]<<"\"!***"<<endl;
			jerr<<"***        make sure the JANA_PLUGIN_PATH environment variable is set correctly."<<endl;
			jerr<<"***        To see paths checked, set PRINT_PLUGIN_PATHS config. parameter"<<endl;
			Unlock();
			exit(-1);
		}
	}
	
	// Append generators back onto appropriate lists
	eventSourceGenerators.insert(eventSourceGenerators.end(), my_eventSourceGenerators.begin(), my_eventSourceGenerators.end());
	factoryGenerators.insert(factoryGenerators.end(), my_factoryGenerators.begin(), my_factoryGenerators.end());
	calibrationGenerators.insert(calibrationGenerators.end(), my_calibrationGenerators.begin(), my_calibrationGenerators.end());

	return RESOURCE_UNAVAILABLE;
}

//---------------------------------
// RecordFactoryCalls
//---------------------------------
jerror_t JApplication::RecordFactoryCalls(JEventLoop *loop)
{
	/// Record the number of calls to each of the factories owned by the
	/// given JEventLoop. This is called (eventually) when the JEventLoop
	/// is deleted so that it contains the final statistics.
	map<string, unsigned int> calls;
	map<string, unsigned int> gencalls;
	vector<JFactory_base*> factories = loop->GetFactories();
	for(unsigned int i=0; i<factories.size(); i++){
		JFactory_base *fac = factories[i];
		string name = fac->GetDataClassName();
		string tag = fac->Tag();
		string nametag = name;
		if(tag != "")nametag += ":" + tag;
		calls[nametag] = fac->GetNcalls();
		gencalls[nametag] = fac->GetNgencalls();
	}
	
	// This should only be called when the app mutex is already locked
	// so we don't need to do it here.
	Nfactory_calls[pthread_self()] = calls;
	Nfactory_gencalls[pthread_self()] = gencalls;

	return NOERROR;
}


//---------------------------------
// PrintFactoryReport
//---------------------------------
jerror_t JApplication::PrintFactoryReport(void)
{
	/// Print a brief report to the screen listing all of the existing
	/// factories and how many calls were made to each.

	// First, get a list of the nametags and thread numbers
	vector<string> nametag;
	vector<pthread_t> thread;
	map<pthread_t, map<string, unsigned int> >::iterator iter=Nfactory_calls.begin();
	for(; iter!=Nfactory_calls.end(); iter++){
		thread.push_back(iter->first);
		if(iter==Nfactory_calls.begin()){
			map<string, unsigned int>::iterator itern = iter->second.begin();
			for(; itern!=iter->second.end(); itern++){
				nametag.push_back(itern->first);
			}
		}
	}
	
	// Print table title and info
	cout<<endl;
	cout<<ansi_bold;
	cout<<"Factory Report:"<<endl;
	cout<<"======================"<<endl;
	cout<<ansi_normal;
	cout<<"The table below contains the number of calls to each factory by thread"<<endl;
	cout<<"Entries are:  \"Num. calls/Num. gens\"    where Num. calls is the number"<<endl;
	cout<<"of times the factory's data objects were requested and Num. gens is"<<endl;
	cout<<"the number of events that the factory actually had to generate objects."<<endl;
	cout<<""<<endl;
	
	// Some parameters to control spacing in table
	unsigned int colwidth = 15;
	unsigned int colshift = 10 + colwidth;
	
	// Check length of factory nametag strings and increase colshift to accomodate
	// the largest one.
	for(unsigned int i=0; i<nametag.size(); i++){
		unsigned int s = nametag[i].size()+3+colwidth;
		if(s>colshift)colshift = s;
	}
	
	unsigned int maxcol = colshift+1+thread.size()*colwidth;
	if(maxcol<80) maxcol = 80;
	
	// Print column headers
	string header1(maxcol,' ');
	string header2(maxcol,' ');
	string facstring = "Factory:";
	header2.replace(0, facstring.size(), facstring);
	string totstring = "Total";
	header2.replace(colshift-colwidth, totstring.size(), totstring);
	for(unsigned int i=0; i<thread.size(); i++){
		stringstream ss;
		ss<<"0x"<<hex<<(unsigned long)thread[i];
		string thridstring = "Thread";
		header2.replace(colshift+i*colwidth, ss.str().size(), ss.str());
		header1.replace(colshift+1+i*colwidth, thridstring.size(), thridstring);
	}
	cout<<endl;
	cout<<header1<<endl;
	cout<<header2<<endl;
	string hr(maxcol,'-');
	cout<<hr<<endl;
	
	// Loop over nametags
	for(unsigned int i=0; i<nametag.size(); i++){
		string &name = nametag[i];
		string line(maxcol,' ');
		line.replace(0, name.size(), name);
		
		// Loop over threads
		unsigned int Ntot = 0;
		unsigned int Ngen_tot = 0;
		for(unsigned int j=0; j<thread.size(); j++){
			unsigned int N = Nfactory_calls[thread[j]][name];
			unsigned int Ngen = Nfactory_gencalls[thread[j]][name];
			stringstream ss;
			ss<<N<<"/"<<Ngen;
			line.replace(colshift+j*colwidth, ss.str().size(), ss.str());
			Ntot += N;
			Ngen_tot += Ngen;
		}
		
		// Add total calls to line
		stringstream ss;
		ss<<Ntot<<"/"<<Ngen_tot;
		line.replace(colshift-colwidth-2, ss.str().size(), ss.str());
		cout<<line<<endl;
	}
	cout<<endl;

	return NOERROR;
}

//---------------------------------
// PrintResourceReport
//---------------------------------
jerror_t JApplication::PrintResourceReport(void)
{
	/// Print a brief report to the screen of the resources 
	/// (memory and CPU usage) used by this process so far
	struct rusage self_usage, child_usage;
	getrusage(RUSAGE_SELF, &self_usage);
	getrusage(RUSAGE_CHILDREN, &child_usage);
	
	double self_sys_seconds = (double)self_usage.ru_stime.tv_sec + (double)self_usage.ru_stime.tv_usec/1000000.0;
	double self_user_seconds = (double)self_usage.ru_utime.tv_sec + (double)self_usage.ru_utime.tv_usec/1000000.0;
	double child_sys_seconds = (double)child_usage.ru_stime.tv_sec + (double)child_usage.ru_stime.tv_usec/1000000.0;
	double child_user_seconds = (double)child_usage.ru_utime.tv_sec + (double)child_usage.ru_utime.tv_usec/1000000.0;
	
	cout<<"Resource Usage Report :"<<endl;
	cout<<endl;

	cout<<"SELF: ------------"<<endl;
	struct rusage *u = &self_usage;
	cout<<"system time usage:"<<self_sys_seconds<<endl;
	cout<<"user time usage:"<<self_user_seconds<<endl;
	cout<<"ru_maxrss="<<u->ru_maxrss<<endl;
	cout<<"ru_ixrss="<<u->ru_ixrss<<endl;
	cout<<"ru_idrss="<<u->ru_idrss<<endl;
	cout<<"ru_isrss="<<u->ru_isrss<<endl;
	cout<<"ru_minflt="<<u->ru_minflt<<endl;
	cout<<"ru_majflt="<<u->ru_majflt<<endl;
	cout<<"ru_nswap="<<u->ru_nswap<<endl;
	cout<<"ru_inblock="<<u->ru_inblock<<endl;
	cout<<"ru_oublock="<<u->ru_oublock<<endl;
	cout<<"ru_msgsnd="<<u->ru_msgsnd<<endl;
	cout<<"ru_msgrcv="<<u->ru_msgrcv<<endl;
	cout<<"ru_nsignals="<<u->ru_nsignals<<endl;
	cout<<"ru_nvcsw="<<u->ru_nvcsw<<endl;
	cout<<"ru_nivcsw="<<u->ru_nivcsw<<endl;

	cout<<"CHILDREN: ------------"<<endl;
	u = &child_usage;
	cout<<"system time usage:"<<child_sys_seconds<<endl;
	cout<<"user time usage:"<<child_user_seconds<<endl;
	cout<<"ru_maxrss="<<u->ru_maxrss<<endl;
	cout<<"ru_ixrss="<<u->ru_ixrss<<endl;
	cout<<"ru_idrss="<<u->ru_idrss<<endl;
	cout<<"ru_isrss="<<u->ru_isrss<<endl;
	cout<<"ru_minflt="<<u->ru_minflt<<endl;
	cout<<"ru_majflt="<<u->ru_majflt<<endl;
	cout<<"ru_nswap="<<u->ru_nswap<<endl;
	cout<<"ru_inblock="<<u->ru_inblock<<endl;
	cout<<"ru_oublock="<<u->ru_oublock<<endl;
	cout<<"ru_msgsnd="<<u->ru_msgsnd<<endl;
	cout<<"ru_msgrcv="<<u->ru_msgrcv<<endl;
	cout<<"ru_nsignals="<<u->ru_nsignals<<endl;
	cout<<"ru_nvcsw="<<u->ru_nvcsw<<endl;
	cout<<"ru_nivcsw="<<u->ru_nivcsw<<endl;

	cout<<endl;
	cout<<endl;

	return NOERROR;
}

//---------------------------------
// GetInstantaneousThreadRates
//---------------------------------
void JApplication::GetInstantaneousThreadRates(map<pthread_t,double> &rates_by_thread)
{
	for(unsigned int i=0; i<threads.size(); i++)rates_by_thread[threads[i]->loop->GetPThreadID()] = threads[i]->loop->GetInstantaneousRate();
}

//---------------------------------
// GetInstegratedThreadRates
//---------------------------------
void JApplication::GetIntegratedThreadRates(map<pthread_t,double> &rates_by_thread)
{
	for(unsigned int i=0; i<threads.size(); i++)rates_by_thread[threads[i]->loop->GetPThreadID()] = threads[i]->loop->GetIntegratedRate();
}

//---------------------------------
// GetThreadNevents
//---------------------------------
void JApplication::GetThreadNevents(map<pthread_t,unsigned int> &Nevents_by_thread)
{
	for(unsigned int i=0; i<threads.size(); i++)Nevents_by_thread[threads[i]->loop->GetPThreadID()] = threads[i]->loop->GetNevents();
}

//---------------------------------
// GetThreadID
//---------------------------------
pthread_t JApplication::GetThreadID(unsigned int index)
{
	pthread_t thr = 0x0;

	WriteLock("app");
	if(index<threads.size())thr = threads[index]->thread_id;
	Unlock("app");
	
	return thr;
}

//---------------------------------
// SignalThreads
//---------------------------------
void JApplication::SignalThreads(int signo)
{
	// Send signal "signo" to all processing threads.
	for(unsigned int i=0; i<threads.size(); i++){
		pthread_kill(threads[i]->thread_id, signo);
	}
}

//---------------------------------
// KillThread
//---------------------------------
bool JApplication::KillThread(pthread_t thr, bool verbose)
{
	bool found_thread = false;

	WriteLock("app");
	for(unsigned int i=0; i<threads.size(); i++){
		if(threads[i]->thread_id==thr){
			pthread_kill(thr, SIGHUP);
			found_thread = true;
			break;
		}
	}
	Unlock("app");
	
	if(verbose){
		if(found_thread){
			jout<<"Killing thread: "<<thr<<endl;
		}else{
			jout<<"Thread "<<thr<<" not found (and so, not killed)"<<endl;
		}
	}
	
	return found_thread;
}

//---------------------------------
// SetNthreads
//---------------------------------
void JApplication::SetNthreads(int new_Nthreads)
{
	/// Set the number of desired processing threads. The
	/// actual number of threads will be adjusted in the
	/// Run() method.
	///
	/// Note that this may also increase the value of MAX_RELAUNCH_THREADS.
	/// This is because the algorithm used in the monitoring thread to check
	/// for dead threads treats any deficit of threads (target number of threads
	/// minus currently running threads) as though the threads died and new
	/// ones need to be launched. The MAX_RELAUNCH_THREADS configuration parameter
	/// is to allow the user the ability to limit the number of automatic
	/// relaunches that can happen. The default of this is zero which makes
	/// some sense in an offline environment where one is interested in fixing all
	/// cases where a thread crash occurs.
	///
	/// Presumably, the user does not want the program to
	/// exit when calling this routine so we make sure that MAX_RELAUNCH_THREADS
	/// is increased by any defict. It will, however, not decrease the value
	/// of MAX_RELAUNCH_THREADS if the setting the number of threads to
	/// something smaller than what is currently running.
	
	int deficit = new_Nthreads - this->Nthreads;
	if(deficit > 0) MAX_RELAUNCH_THREADS += deficit;

	// Now set the new target number of threads
	this->Nthreads = new_Nthreads;
	jout<<"Setting number of processing threads to: "<<this->Nthreads<<endl;
}

//---------------------------------
// RegisterHUPMutex
//---------------------------------
void JApplication::RegisterHUPMutex(pthread_mutex_t *mutex)
{
	/// Register a pthread_mutex_t to be unlocked inside the default signal
	/// handler for HUP signals. The mutex should be of type PTHREAD_MUTEX_ERRORCHECK
	/// so if an unlock attempt is made when it is not locked or locked
	/// by a different thread, then the program will continue without crashing.
	/// No check can be made to verify the type so it is up to the user to
	/// ensure this.
	/// The main purpose of this is to release a mutex that may be held by a
	/// thread that is told to die via SIGHUP (e.g. while writing event to file).
	/// One can also register a callback routine where more complicated handling
	/// can be perfromed. Note that all registered mutexes will be unlocked first
	/// before any callbacks are called.
	HUP_locks.push_back(mutex);
}

//---------------------------------
// RegisterHUPCallback
//---------------------------------
void JApplication::RegisterHUPCallback(CallBack_t *callback, void *arg)
{
	/// Register a callback routine that will be called from the default
	/// signal handler for HUP signals. The framework will send an HUP
	/// signal when a thread has stalled or the number of processing
	/// threads is being reduced. This allows the user to do additional
	/// cleanup to handle the HUP signal.
	///
	/// NOTE: For the special case where all that is needed is to unlock
	/// a mutex, the RegisterHUPMutex method can be used instead and a
	/// callback need not be provided. The callback routine should have
	/// a form like this:
	///
	///  void MyHUPCallback(void *arg){ ... }
	///
	/// Then it can be registered like this:
	///
	///   japp->RegisterHUPCallback(MyHUPCallback, (void*)&my_data) { ... }
	///
	/// Note that all registered mutexes will be unlocked first
	/// before any callbacks are called.
	HUP_callbacks.push_back(pair<CallBack_t*, void*>(callback, arg));
}

//---------------------------------
// SetStatusBitDescription
//---------------------------------
void JApplication::SetStatusBitDescription(uint32_t bit, string description)
{
	/// Set the description of the specified bit.
	/// The value of "bit" should be from 0-63.
	
	WriteLock("status_bit_descriptions");
	status_bit_descriptions[bit] = description;
	Unlock("status_bit_descriptions");
}

//---------------------------------
// GetStatusBitDescription
//---------------------------------
string JApplication::GetStatusBitDescription(uint32_t bit)
{
	/// Get the description of the specified status bit.
	/// The value of "bit" should be from 0-63.
	
	string description("no description available");

	ReadLock("status_bit_descriptions");
	map<uint32_t, string>::iterator iter = status_bit_descriptions.find(bit);
	if(iter != status_bit_descriptions.end()) description = iter->second;
	Unlock("status_bit_descriptions");
	
	return description;
}

//---------------------------------
// GetStatusBitDescriptions
//---------------------------------
void JApplication::GetStatusBitDescriptions(map<uint32_t, string> &status_bit_descriptions)
{
	/// Get the full list of descriptions of status bits.
	/// Note that the meaning of the bits is implementation
	/// specific and so descriptions are optional. It may be
	/// that some or none of the bits used have an associated description.
	
	ReadLock("status_bit_descriptions");
	status_bit_descriptions = this->status_bit_descriptions;
	Unlock("status_bit_descriptions");
}

//---------------------------------
// StatusWordToString
//---------------------------------
string JApplication::StatusWordToString(uint64_t status)
{
	/// Given a 64-bit status word, generate a nicely formatted string
	/// suitable for printing to the screen. This will include the
	/// entire word in both hexadecimal and binary. It will also print
	/// 
	
	// Lock mutex to prevent changes to status_bit_descriptions while we
	// read from it.
	ReadLock("status_bit_descriptions");
	
	stringstream ss;
	
	// Add status in hex first
	ss << "status: 0x" << hex << setw(sizeof(uint64_t)*2) << setfill('0') << status << dec <<endl;
	
	// Binary
	ss << setw(0) << "   bin |";
	for(int i=sizeof(uint64_t)*8-1; i>=0; i--){
		ss << ((status>>i) & 0x1);
		if((i%8)==0) ss << "|";
	}
	ss << endl;
	
	// 1-byte hex under binary
	ss << "   hex ";
	for(int i=sizeof(uint64_t) - 1; i>=0; i--){
		ss << hex << "   0x"<< setw(2) << ((status>>(i*8)) & 0xFF) << "  ";
	}
	ss << endl;
	
	// Descriptions for each bit that has a description or is set
	for(unsigned int i=0; i<sizeof(uint64_t)*8; i++){
		uint64_t val = ((status>>i) & 0x1);
		
		map<uint32_t, string>::iterator iter = status_bit_descriptions.find(i);
		
		if(iter != status_bit_descriptions.end() || val != 0){
			ss << dec << setw(2) << setfill(' ');
			ss << " " << val << " - [" << setw(2) << setfill(' ') << i << "] " << GetStatusBitDescription(i) << endl;
		}
	}
	ss << endl;
	
	// Release lock
	Unlock("status_bit_descriptions");

	return ss.str();
}




