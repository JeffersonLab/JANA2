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

#include <signal.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <dirent.h>

#include "JEventLoop.h"
#include "JApplication.h"
#include "JEventProcessor.h"
#include "JEventSource.h"
#include "JEvent.h"
#include "JGeometry.h"
#include "JParameterManager.h"
#include "JLog.h"
#include "JCalibrationFile.h"

void* LaunchEventBufferThread(void* arg);
void* LaunchThread(void* arg);

JLog dlog;
JApplication *japp = NULL;

int SIGINT_RECEIVED = 0;
int SIGUSR1_RECEIVED = 0;
int NTHREADS_COMMAND_LINE = 0;

//-----------------------------------------------------------------
// ctrlCHandle
//-----------------------------------------------------------------
void ctrlCHandle(int x)
{
	SIGINT_RECEIVED++;
	cerr<<endl<<"SIGINT received ("<<SIGINT_RECEIVED<<")....."<<endl;
	if(SIGINT_RECEIVED == 3){
		cerr<<endl<<"Three SIGINTS received! Attempting graceful exit ..."<<endl<<endl;
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
	cerr<<endl<<"SIGUSR1 received ("<<SIGUSR1_RECEIVED<<").....(thread=0x"<<hex<<pthread_self()<<dec<<")"<<endl;
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
	cerr<<endl<<"SIGUSR2 received .....(thread=0x"<<hex<<pthread_self()<<dec<<")"<<endl;

#ifdef __linux__
	void * array[50];
	int nSize = backtrace(array, 50);
	char ** symbols = backtrace_symbols(array, nSize);

	cout<<endl;
	cout<<"--- Stack trace for thread=0x"<<hex<<pthread_self()<<dec<<"): ---"<<endl;
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
	system(cmd.c_str());
	cout<<endl;
#else
	cerr<<"Stack trace only supported on Linux at this time"<<endl;
#endif
	japp->Unlock();
	exit(0);
}

//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(int narg, char* argv[])
{
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
	pthread_mutex_init(&app_mutex, NULL);
	pthread_mutex_init(&current_source_mutex, NULL);
	pthread_mutex_init(&geometry_mutex, NULL);
	pthread_mutex_init(&calibration_mutex, NULL);
	pthread_mutex_init(&event_buffer_mutex, NULL);

	// Variables used for calculating the rate
	show_ticker = 1;
	NEvents = 0;
	last_NEvents = 0;
	avg_NEvents = 0;
	avg_time = 0.0;
	rate_instantaneous = 0.0;
	rate_average = 0.0;
	monitor_heartbeat= true;
	init_called = false;
	
	// Default plugin search path
	AddPluginPath(".");
	
	// Configuration Parameter manager
	jparms = new JParameterManager();
	
	// Event buffer
	event_buffer_filling = true;
	
	// Sources
	current_source = NULL;
	for(int i=1; i<narg; i++){
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
				cerr<<__FILE__<<":"<<__LINE__<<" bad parameter argument ("<<argv[i]<<") should be of form -Pkey=value"<<endl;
			}
			free(pstr);
			continue;
		}
		if(argv[i][0] == '-')continue;
		source_names.push_back(argv[i]);
	}
	
	// Global variable
	japp = this;
}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{
	for(unsigned int i=0; i<geometries.size(); i++)delete geometries[i];
	geometries.clear();
	for(unsigned int i=0; i<calibrations.size(); i++)delete calibrations[i];
	calibrations.clear();
	for(unsigned int i=0; i<heartbeats.size(); i++)delete heartbeats[i];
	heartbeats.clear();
}

//---------------------------------
// NextEvent
//---------------------------------
jerror_t JApplication::NextEvent(JEvent &event)
{
	/// Grab an event from the event buffer. If no events are
	/// there, it will loop until:
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
		}
		pthread_mutex_unlock(&event_buffer_mutex);
		if(event.GetJEventLoop()->GetQuit())break;
	}while((myevent==NULL) && event_buffer_filling);
	
	// If we managed to get an event, copy it to the given
	// reference and delete the JEvent object
	if(myevent){
		// It's tempting to just copy the *myevent JEvent object into
		// event directly. This will overwrite the "loop" member
		// which we don't want. Copy the other members by hand.
		event.SetJEventSource(myevent->GetJEventSource());
		event.SetRunNumber(myevent->GetRunNumber());
		event.SetEventNumber(myevent->GetEventNumber());
		event.SetRef(myevent->GetRef());
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
	/// JApplication pointe passed in through arg
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
	/// It exits once it has read the last event.
	jerror_t err;
	unsigned int sleep_time=10;
	int Niterations=0;
	int Nread=0;
	do{
		Niterations++;

		pthread_mutex_lock(&event_buffer_mutex);
		unsigned int Nevents_in_buffer = event_buffer.size();
		pthread_mutex_unlock(&event_buffer_mutex);
		
		unsigned int MAX_EVENTS_IN_BUFFER = 10;
		err = NOERROR;
		if(Nevents_in_buffer<MAX_EVENTS_IN_BUFFER){
			JEvent *event = new JEvent;
			err = ReadEvent(*event);

			if(err==NOERROR){
				pthread_mutex_lock(&event_buffer_mutex);
				event_buffer.push_front(event);
				pthread_mutex_unlock(&event_buffer_mutex);
				Nevents_in_buffer++;
				Nread++;
			}else{
				delete event;
			}
		}
		
		// If we know the buffer is not full, then don't sleep
		// at all and go right into the next iteration
		if(Nevents_in_buffer<MAX_EVENTS_IN_BUFFER && err==NOERROR)continue;
		
		// Here we want to sleep a little to avoid wasting a lot
		// of CPU time checking the event buffer at a rate
		// much more frequent than it is emptied.It turns out that
		// if we sleep too little, it will severely degrade the 
		// performance on a single processor machine. Sleeping
		// too much is obviously bad since it would cause 
		// the event consumers to go into a holding pattern
		// waiting for events. Therefore, we keep a tally of
		// how often we actually needed to read in an event
		// every 100 times through this loop. The sleep time
		// is then adjusted to try and maintain that at 50%
		if(Niterations>100){
			double s = (double)Nread/(double)Niterations + 1.0E-6;
			sleep_time = (int)((double)sleep_time*0.5/s);
			if(sleep_time<1)sleep_time=1;
			if(sleep_time>100000)sleep_time=100000;
			Nread = Niterations = 0;
		}
		
		usleep(sleep_time);

	}while(err!=NO_MORE_EVENT_SOURCES);

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

	pthread_mutex_lock(&current_source_mutex);
	if(!current_source){
		jerror_t err = OpenNext();
		pthread_mutex_unlock(&current_source_mutex);
		if(err != NOERROR)return err;
		return ReadEvent(event);
	}
	
	// Read next event from source. If none, then simply set current_source
	// to NULL and recall ourself
	jerror_t err;
	try{
		err = current_source->GetEvent(event);
	}catch(...){
		// If we get an exception, consider this source finished!
		err = NO_MORE_EVENTS_IN_SOURCE;
	}
	switch(err){
		case NO_MORE_EVENTS_IN_SOURCE:
			current_source = NULL;
			pthread_mutex_unlock(&current_source_mutex);
			return ReadEvent(event);
			break;
		default:
			break;
	}

	// Event counter
	NEvents++;

	pthread_mutex_unlock(&current_source_mutex);

	return NOERROR;
}

//---------------------------------
// GetEventBufferSize
//---------------------------------
unsigned int JApplication::GetEventBufferSize(void)
{
	pthread_mutex_lock(&event_buffer_mutex);
	unsigned int Nevents_in_buffer = event_buffer.size();
	pthread_mutex_unlock(&event_buffer_mutex);

	return Nevents_in_buffer;
}

//---------------------------------
// AddProcessor
//---------------------------------
jerror_t JApplication::AddProcessor(JEventProcessor *processor)
{
	processor->SetJApplication(this);
	processors.push_back(processor);

	return NOERROR;
}

//---------------------------------
// RemoveProcessor
//---------------------------------
jerror_t JApplication::RemoveProcessor(JEventProcessor *processor)
{
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
jerror_t JApplication::AddJEventLoop(JEventLoop *loop, double* &heartbeat)
{
	pthread_mutex_lock(&app_mutex);
	loops.push_back(loop);

	// Loop over all factory generators, creating the factories
	// for this JEventLoop.
	for(unsigned int i=0; i<factoryGenerators.size(); i++){
		factoryGenerators[i]->GenerateFactories(loop);
	}
	
	// For the heartbeat, we use a double that the thread should
	// set to zero each event. It will be added to periodically 
	// in Run() (see below) by the main thread so as to keep track of
	// the amount of time in seconds the thread has been inactive.
	heartbeat = new double;
	*heartbeat = 0.0;
	heartbeats.push_back(heartbeat);
	
	pthread_mutex_unlock(&app_mutex);

	return NOERROR;
}

//---------------------------------
// RemoveJEventLoop
//---------------------------------
jerror_t JApplication::RemoveJEventLoop(JEventLoop *loop)
{
	vector<JEventLoop*>::iterator iter = loops.begin();
	vector<double*>::iterator hbiter = heartbeats.begin();
	for(; iter!=loops.end(); iter++, hbiter++){
		if((*iter) == loop){
			loops.erase(iter);
			heartbeats.erase(hbiter);
			break;
		}
	}

	return NOERROR;
}

//---------------------------------
// AddEventSourceGenerator
//---------------------------------
jerror_t JApplication::AddEventSourceGenerator(JEventSourceGenerator *generator)
{
	eventSourceGenerators.push_back(generator);

	return NOERROR;
}

//---------------------------------
// RemoveEventSourceGenerator
//---------------------------------
jerror_t JApplication::RemoveEventSourceGenerator(JEventSourceGenerator *generator)
{
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
	factoryGenerators.push_back(generator);

	return NOERROR;
}

//---------------------------------
// RemoveFactoryGenerator
//---------------------------------
jerror_t JApplication::RemoveFactoryGenerator(JFactoryGenerator *generator)
{
	vector<JFactoryGenerator*>& f = factoryGenerators;
	vector<JFactoryGenerator*>::iterator iter = find(f.begin(), f.end(), generator);
	if(iter != f.end())f.erase(iter);

	return NOERROR;
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
		if((*iter)->IsInRange(run_number)){
			// Found it! Unlock mutex and return pointer
			JGeometry *g = *iter;
			pthread_mutex_unlock(&geometry_mutex);
			return g;
		}
	}
	
	// JGeometry object for this run_number doesn't exist in our list.
	// Create a new one and add it to the list.
	JGeometry *g = new JGeometry(run_number);
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

	// Lock mutex to keep list from being modified while we search it
	pthread_mutex_lock(&calibration_mutex);

	vector<JCalibration*>::iterator iter = calibrations.begin();
	for(; iter!=calibrations.end(); iter++){
		if((*iter)->GetRunMin()>(int)run_number)continue;
		if((*iter)->GetRunMax()<(int)run_number)continue;
		// Found it! Unlock mutex and return pointer
		JCalibration *g = *iter;
		pthread_mutex_unlock(&calibration_mutex);
		return g;
	}
	
	// JCalibration object for this run_number doesn't exist in our list.
	// Create a new one and add it to the list.
	// We need to create an object of the appropriate subclass of
	// JCalibration. This determined by the first several characters
	// of the URL that specifies the calibration database location.
	// For now, only the JCalibrationFile subclass exists so we'll
	// just make one of those and defer the heavier algorithm until
	// later.
	const char *url = getenv("JANA_CALIB_URL");
	if(!url)url="file://./";
	const char *context = getenv("JANA_CALIB_CONTEXT");
	if(!context)context="default";
	JCalibration *g = new JCalibrationFile(string(url), run_number, context);
	if(g)calibrations.push_back(g);

	// Unlock calibration mutex
	pthread_mutex_unlock(&calibration_mutex);

	return g;
}

//----------------
// LaunchThread
//----------------
void* LaunchThread(void* arg)
{
	/// This is a global function that is used to create
	/// a new JEventLoop object which runs in its own thread.

	// We don't want event processing threads handling
	// the SIGUSR1 signals. They should be handled by the main thread
	//sigset_t set;
	//sigemptyset(&set);
	//sigaddset(&set, SIGUSR2);
	//pthread_sigmask(SIG_BLOCK, &set, NULL);

	// Create JEventLoop object. He automatically registers himself
	// with the JApplication object. 
	JEventLoop *eventLoop = new JEventLoop((JApplication*)arg);

	// Loop over events until done. Catch any jerror_t's thrown
	try{
		eventLoop->Loop();
	}catch(JException *exception){
		if(exception)delete exception;
		cerr<<__FILE__<<":"<<__LINE__<<" EXCEPTION caught for thread "<<pthread_self()<<endl;
	}

	// Delete JEventLoop object. He automatically de-registers himself
	// with the JEventLoop Object
	delete eventLoop;

	pthread_exit(arg);

	return arg;
}

//---------------------------------
// Init
//---------------------------------
jerror_t JApplication::Init(void)
{
	if(init_called)return NOERROR; // don't initialize twice1
	init_called = true;

	// Attach any plugins
	AttachPlugins();

	// Call init Processors (note: factories don't exist yet)
	try{
		for(unsigned int i=0;i<processors.size();i++)processors[i]->init();
	}catch(jerror_t err){
		cerr<<endl;
		cerr<<__FILE__<<":"<<__LINE__<<" Error thrown ("<<err<<") from JEventProcessor::init()"<<endl;
		exit(-1);
	}

	// Launch event buffer thread
	pthread_t ebthr;
	pthread_create(&ebthr, NULL, LaunchEventBufferThread, this);
	
	return NOERROR;
}

//---------------------------------
// Run
//---------------------------------
jerror_t JApplication::Run(JEventProcessor *proc, int Nthreads)
{
	// If a JEventProcessor was passed, then add it to our list first
	if(proc){
		jerror_t err = AddProcessor(proc);
		if(err)return err;
	}

	// Call init() for JEventProcessors (factories don't exist yet)
	Init();
	
	// Launch all threads
	if(Nthreads<1){
		// If Nthreads is less than 1 then automatically set to 1
		Nthreads = 1;
	}
	if(NTHREADS_COMMAND_LINE>0){
		Nthreads = NTHREADS_COMMAND_LINE;
	}
	cout<<"Launching threads "; cout.flush();
	for(int i=0; i<Nthreads; i++){
		pthread_t thr;
		pthread_create(&thr, NULL, LaunchThread, this);
		threads.push_back(thr);
		cout<<".";cout.flush();
	}
	cout<<endl;
	
	// Do a sleepy loop so the threads can do their work
	struct timespec req, rem;
	req.tv_nsec = (int)0.5E9; // set to 1/2 second
	req.tv_sec = 0;
	double sleep_time = (double)req.tv_sec + (1.0E-9)*(double)req.tv_nsec;
	do{
		// Sleep for a specific amount of time and calculate the rate
		// on each iteration through the loop
		rem.tv_sec = rem.tv_nsec = 0;
		nanosleep(&req, &rem);
		if(rem.tv_sec == 0 && rem.tv_nsec == 0){
			// If there was no time remaining, then we must have slept
			// the whole amount
			int delta_NEvents = NEvents - last_NEvents;
			avg_NEvents += delta_NEvents;
			avg_time += sleep_time;
			rate_instantaneous = (double)delta_NEvents/sleep_time;
			rate_average = (double)avg_NEvents/avg_time;
		}else{
			cout<<__FILE__<<":"<<__LINE__<<" didn't sleep full "<<sleep_time<<" seconds!"<<endl;
		}
		last_NEvents = NEvents;
		
		// If show_ticker is set, then update the screen with the rate(s)
		if(show_ticker && loops.size()>0)PrintRate();
		
		if(SIGINT_RECEIVED)Quit();
		
		// Add time slept to all heartbeats
		double rem_time = (double)rem.tv_sec + (1.0E-9)*(double)rem.tv_nsec;
		double slept_time = sleep_time - rem_time;
		for(unsigned int i=0;i<heartbeats.size();i++){
			double *hb = heartbeats[i];
			*hb += slept_time;
			if(monitor_heartbeat && (*hb > 7.0+sleep_time)){
				// Thread hasn't done anything for more than 2 seconds. 
				// Remove it from monitoring lists.
				cerr<<" Thread "<<i<<" hasn't responded in "<<*hb<<" seconds. Delisting it..."<<endl;
				JEventLoop *loop = *(loops.begin()+i);
				loops.erase(loops.begin()+i);
				heartbeats.erase(heartbeats.begin()+i);
				
				// Signal the non-responsive thread to commit suicide
				pthread_kill(loop->GetPThreadID(), SIGHUP);
			}
		}

		// When a JEventLoop runs out of events, it removes itself from
		// the list before returning from the thread.
	}while(loops.size() > 0);
	
	// Call erun() and fini() methods and delete event sources
	Fini();
	
	// Merge up all the threads
	for(unsigned int i=0; i<threads.size(); i++){
		void *ret;
		cout<<"Merging thread "<<i<<" ..."<<endl; cout.flush();
		pthread_join(threads[i], &ret);
	}
	
	// Close any open dll's
	for(unsigned int i=0; i<sohandles.size(); i++){
		cout<<"Closing shared object handle "<<i<<" ..."<<endl; cout.flush();
		dlclose(sohandles[i]);
	}
	
	cout<<" "<<NEvents<<" events processed. Average rate: "
		<<Val2StringWithPrefix(rate_average)<<"Hz"<<endl;

	return NOERROR;
}

//---------------------------------
// Fini
//---------------------------------
jerror_t JApplication::Fini(void)
{
	// Make sure erun is called
	for(unsigned int i=0;i<processors.size();i++){
		JEventProcessor *proc = processors[i];
		if(proc->brun_was_called() && !proc->erun_was_called()){
			try{
				proc->erun();
			}catch(jerror_t err){
				cerr<<endl;
				cerr<<__FILE__<<":"<<__LINE__<<" Error thrown ("<<err<<") from JEventProcessor::erun()"<<endl;
			}
			proc->Set_erun_called();
		}
	}

	// Call fini Processors
	try{
		for(unsigned int i=0;i<processors.size();i++)processors[i]->fini();
	}catch(jerror_t err){
		cerr<<endl;
		cerr<<__FILE__<<":"<<__LINE__<<" Error thrown ("<<err<<") from JEventProcessor::fini()"<<endl;
	}
	
	// Delete all sources allowing them to close cleanly
	for(unsigned int i=0;i<sources.size();i++)delete sources[i];
	sources.clear();

	return NOERROR;
}

//---------------------------------
// Pause
//---------------------------------
void JApplication::Pause(void)
{
	vector<JEventLoop*>::iterator iter = loops.begin();
	for(; iter!=loops.end(); iter++){
		(*iter)->Pause();
	}
}

//---------------------------------
// Resume
//---------------------------------
void JApplication::Resume(void)
{
	vector<JEventLoop*>::iterator iter = loops.begin();
	for(; iter!=loops.end(); iter++){
		(*iter)->Resume();
	}
}

//---------------------------------
// Quit
//---------------------------------
void JApplication::Quit(void)
{
	cout<<endl<<"Telling all threads to quit ..."<<endl;
	vector<JEventLoop*>::iterator iter = loops.begin();
	for(; iter!=loops.end(); iter++){
		(*iter)->Quit();
	}
}

//----------------
// Val2StringWithPrefix
//----------------
string JApplication::Val2StringWithPrefix(float val)
{
	char *units = "";
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
	string event_str = Val2StringWithPrefix(NEvents) + " events";
	string ir_str = Val2StringWithPrefix(rate_instantaneous) + "Hz";
	string ar_str = Val2StringWithPrefix(rate_average) + "Hz";
	cout<<"  "<<event_str<<"   "<<ir_str
		<<"  (average rate: "<<ar_str<<")"
		//<<"  event buffer:"<<GetEventBufferSize()
		<<"     \r";
	cout.flush();
}

//---------------------------------
// OpenNext
//---------------------------------
jerror_t JApplication::OpenNext(void)
{
	/// Open the next source in the list. If there are none,
	/// then return NO_MORE_EVENT_SOURCES
	
	if(sources.size() >= source_names.size())return NO_MORE_EVENT_SOURCES;
	
	// Loop over JEventSourceGenerator objects and find the one
	// (if any) that has the highest chance of being able to read
	// this source. The return value of 
	// JEventSourceGenerator::CheckOpenable(source) is a liklihood that
	// the named source can be read by the JEventSource objects
	// created by the generator. In most cases, the liklihood will
	// be either 0.0 or 1.0. In the case that 2 or more generators return
	// equal liklihoods, the first one in the list will be used.
	JEventSourceGenerator* gen = NULL;
	double liklihood = 0.0;
	const char *sname = source_names[sources.size()];
	for(unsigned int i=0; i<eventSourceGenerators.size(); i++){
		double my_liklihood = eventSourceGenerators[i]->CheckOpenable(sname);
		if(my_liklihood > liklihood){
			liklihood = my_liklihood;
			gen = eventSourceGenerators[i];
		}
	}
	
	current_source = NULL;
	if(gen != NULL){
		cout<<"Opening source \""<<sname<<"\"of type: "<<gen->Description()<<endl;
		current_source = gen->MakeJEventSource(sname);
	}

	if(!current_source){
		cerr<<"Unable to open event source!"<<endl;
	}
	
	// Add source to list (even if it's NULL!)
	sources.push_back(current_source);
	
	return NOERROR;
}

//---------------------------------
// RegisterSharedObject
//---------------------------------
jerror_t JApplication::RegisterSharedObject(const char *soname, bool verbose)
{
	// Open shared object
	void* handle = dlopen(soname, RTLD_LAZY | RTLD_GLOBAL);
	if(!handle){
		if(verbose)cerr<<dlerror()<<endl;
		return NOERROR;
	}
	sohandles.push_back(handle);
	
	// Look for an InitPlugin symbol
	InitPlugin_t *plugin = (InitPlugin_t*)dlsym(handle, "InitPlugin");
	if(plugin){
		cout<<"Initializing plugin \""<<soname<<"\" ..."<<endl;
		(*plugin)(this);
	}else{
		if(verbose)cout<<" --- Nothing useful found in "<<soname<<" ---"<<endl;
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
	cout<<"Looking for plugins in \""<<sodirname<<"\" ..."<<endl;
	DIR *dir = opendir(sodirname.c_str());

	struct dirent *d;
	char full_path[512];
	while((d=readdir(dir))){
		// Try registering the file as a shared object.
		if(!strncmp(d->d_name,".nfs",4)){
			cout<<"Skipping .nfs file :"<<d->d_name<<endl;
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
	/// via the AddPlugin(string path) method.
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
	
	bool printPaths = getenv("JANA_PRINT_PLUGIN_PATHS") != NULL;
	
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
	
	// Loop over plugins
	for(unsigned int j=0; j<plugins.size(); j++){
		// Sometimes, the user will include the ".so" suffix in the
		// plugin name. If they don't, then we add it here.
		string plugin = plugins[j];
		if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";
	
		// Loop over paths
		for(unsigned int i=0; i< pluginPaths.size(); i++){
			string fullpath = pluginPaths[i] + "/" + plugin;
			ifstream f(fullpath.c_str());
			if(f.is_open()){
				f.close();
				RegisterSharedObject(fullpath.c_str());
				break;
			}
			if(printPaths) cout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
			
			if(fullpath[0] != '/')continue;
			fullpath = pluginPaths[i] + "/" + plugins[j] + "/" + plugin;
			f.open(fullpath.c_str());
			if(f.is_open()){
				f.close();
				RegisterSharedObject(fullpath.c_str());
				break;
			}
			if(printPaths) cout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
		}
	}

	return RESOURCE_UNAVAILABLE;
}

//---------------------------------
// SignalThreads
//---------------------------------
void JApplication::SignalThreads(int signo)
{
	// Send signal "signo" to all processing threads.
	for(unsigned int i=0; i<threads.size(); i++){
		pthread_kill(threads[i], signo);
	}
}
