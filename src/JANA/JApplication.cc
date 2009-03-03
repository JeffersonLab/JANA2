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
#include <sys/resource.h>

#include "JEventLoop.h"
#include "JApplication.h"
#include "JEventProcessor.h"
#include "JEventSource.h"
#include "JEvent.h"
#include "JGeometryXML.h"
#include "JGeometryMYSQL.h"
#include "JParameterManager.h"
#include "JLog.h"
#include "JCalibrationFile.h"
#include "JVersion.h"
using namespace jana;

#ifndef ansi_escape
#define ansi_escape			((char)0x1b)
#define ansi_bold 			ansi_escape<<"[1m"
#define ansi_normal			ansi_escape<<"[0m"
#endif // ansi_escape

void* LaunchEventBufferThread(void* arg);
void* LaunchThread(void* arg);
void  CleanupThread(void* arg);

JLog dlog;
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
	/// JApplication constructor
	///
	/// The arguments passed here are typically those passed into the program
	/// through <i>main()</i>. The argument list is parsed and any arguments
	/// not relevant for JANA are quietly ignored. This is the
	/// only constructor available for JApplication.

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
	pthread_mutex_init(&geometry_mutex, NULL);
	pthread_mutex_init(&calibration_mutex, NULL);
	pthread_mutex_init(&event_buffer_mutex, NULL);
	pthread_cond_init(&event_buffer_cond, NULL);

	// Variables used for calculating the rate
	show_ticker = 1;
	NEvents_read = 0;
	NEvents = 0;
	last_NEvents = 0;
	avg_NEvents = 0;
	avg_time = 0.0;
	rate_instantaneous = 0.0;
	rate_average = 0.0;
	monitor_heartbeat= true;
	init_called = false;
	fini_called = false;
	stop_event_buffer = false;
	
	// Default plugin search path
	AddPluginPath(".");
	
	// Configuration Parameter manager
	jparms = new JParameterManager();
	
	// Event buffer
	event_buffer_filling = true;
	
	print_factory_report = false;
	print_resource_report = false;
	
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
		arg="--config";
		if(!strncmp(arg, argv[i],strlen(arg))){
			string fname(&argv[i][strlen(arg)+1]);
			ReadConfigFile(fname);
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
				cerr<<__FILE__<<":"<<__LINE__<<" bad parameter argument ("<<argv[i]<<") should be of form -Pkey=value"<<endl;
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
	cout<<"  --dumpcalibrations       Dump calibrations used in \"calib\" directory at end of job"<<endl;
	cout<<"  --resourcereport         Dump a short report on system resources used at end of job"<<endl;
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

	for(unsigned int i=0; i<geometries.size(); i++)delete geometries[i];
	geometries.clear();
	for(unsigned int i=0; i<calibrations.size(); i++)delete calibrations[i];
	calibrations.clear();
	for(unsigned int i=0; i<heartbeats.size(); i++)delete heartbeats[i];
	heartbeats.clear();
}

//---------------------------------
// ReadConfigFile
//---------------------------------
void JApplication::ReadConfigFile(string fname)
{
	/// Read in the configuration file with name specified by "fname".
	/// The file should have the form:
	///
	/// <pre>
	/// key1 value1
	/// key2 value2
	/// ...
	/// </pre>
	/// 
	/// Where there is a space between the key and the value (thus, the "key"
	/// can contain no spaces). The value is taken as the rest of the line
	/// up to, but not including the newline itself.
	///
	/// A key may be specified with no value and the value will be set to "1".
	///
	/// A "#" charater will discard the remaining characters in a line up to
	/// the next newline. Therefore, lines starting with "#" are ignored
	/// completely.
	///
	/// Lines with no characters (except for the newline) are ignored.

	// Try and open file
	ifstream ifs(fname.c_str());
	if(!ifs.is_open()){
		cerr<<"Unable to open configuration file \""<<fname<<"\" !"<<endl;
		return;
	}
	cout<<"Reading configuration from \""<<fname<<"\" ..."<<endl;
	
	// Loop over lines
	char line[1024];
	while(!ifs.eof()){
		// Read in next line ignoring comments 
		ifs.getline(line, 1024);
		if(strlen(line)==0)continue;
		if(line[0] == '#')continue;
		string str(line);

		// Check for comment character and erase comment if found
		if(str.find('#')!=str.npos)str.erase(str.find('#'));

		// Break line into tokens
		vector<string> tokens;
		string buf; // Have a buffer string
		stringstream ss(str); // Insert the string into a stream
		while (ss >> buf)tokens.push_back(buf);
		if(tokens.size()<1)continue; // ignore empty lines

		// Use first token as key
		string key = tokens[0];
		
		// Concatenate remaining tokens into val string
		string val="";
		for(unsigned int i=1; i<tokens.size(); i++){
			if(i!=1)val += " ";
			val += tokens[i];
		}
		if(val=="")val="1";

		// Set Configuration Parameter
		jparms->SetParameter(key, val);
	}
	
	// close file
	ifs.close();	
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
	
	unsigned int EVENTS_TO_SKIP=0;
	unsigned int EVENTS_TO_KEEP=0;
	jparms->SetDefaultParameter("EVENTS_TO_SKIP", EVENTS_TO_SKIP);
	jparms->SetDefaultParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP);
	
	unsigned int MAX_EVENTS_IN_BUFFER = 10;
	jerror_t err;
	JEvent *event = NULL;
	do{
		// Lock mutex
		pthread_mutex_lock(&event_buffer_mutex);
		
		// The "event" pointer actually gets created below, but waits to get
		// pushed onto the event_buffer list until now to save locking and
		// unlocking the mutex one time.
		if(event!=NULL)event_buffer.push_front(event);
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
			if(NEvents_read<(int)EVENTS_TO_SKIP){
				event->FreeEvent();
				delete event;
				event = NULL;
			}
		}
		
		// If the user specified a fixed number of events to keep, then 
		// check that here and end the loop once we've read them all
		// in.
		if(EVENTS_TO_KEEP>0)
			if(NEvents_read >= (int)(EVENTS_TO_SKIP+EVENTS_TO_KEEP))break;
		
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
		err = current_source->GetEvent(event);
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
jerror_t JApplication::AddProcessor(JEventProcessor *processor)
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
jerror_t JApplication::AddJEventLoop(JEventLoop *loop, double* &heartbeat)
{
	/// Add a JEventLoop object and generate a complete set of factories
	/// for it by calling the GenerateFactories method for each of the
	/// JFactoryGenerator objects registered via AddFactoryGenerator().
	/// This is typically not called directly but rather, called by
	/// the JEventLoop constructor.

	pthread_mutex_lock(&app_mutex);
	loops.push_back(loop);
	
	// We need the threads vector to stay in sync with the loops and heartbeats
	// vectors so we push it on here even though it might seem more natural
	// to do this from the main thread when this thread was created.
	threads.push_back(pthread_self());

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
	/// Remove a JEventLoop object from the application. This is typically
	/// not called directly by the user. Rather, it is called from the
	/// JEventLoop destructor.

	pthread_mutex_lock(&app_mutex);
	
	for(unsigned int i=0; i<loops.size(); i++){
		if(loops[i] == loop){
			if(print_factory_report)RecordFactoryCalls(loop);
			loops.erase(loops.begin()+i);
			heartbeats.erase(heartbeats.begin()+i);
			threads.erase(threads.begin()+i);
			break;
		}
	}

	pthread_mutex_unlock(&app_mutex);

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
	// JCalibration. This determined by looking through the 
	// existing JCalibrationGenerator objects and finding the
	// which claims the highest probability of being able to 
	// open it based on the URL. If there are no generators
	// claiming a non-zero probability and the URL starts with
	// "file://", then a JCalibrationFile object is created
	// (i.e. we don't bother making a JCalibrationGeneratorFile
	// class and instead, handle it here.)
	const char *url = getenv("JANA_CALIB_URL");
	if(!url)url="file://./";
	const char *context = getenv("JANA_CALIB_CONTEXT");
	if(!context)context="default";
	
	JCalibrationGenerator* gen = NULL;
	double liklihood = 0.0;
	for(unsigned int i=0; i<calibrationGenerators.size(); i++){
		double my_liklihood = calibrationGenerators[i]->CheckOpenable(string(url), run_number, context);
		if(my_liklihood > liklihood){
			liklihood = my_liklihood;
			gen = calibrationGenerators[i];
		}
	}

	// Make the JCalibration object
	JCalibration *g=NULL;
	if(gen){
		g = gen->MakeJCalibration(string(url), run_number, context);
	}
	if(gen==NULL && !strncmp(url, "file://", 7)){
		g = new JCalibrationFile(string(url), run_number, context);
	}
	if(g){
		calibrations.push_back(g);
		cout<<"Created JCalibration object of type: "<<g->className()<<endl;
		cout<<"Generated via: "<< (gen==NULL ? "fallback creation of JCalibrationFile":gen->Description())<<endl;
		cout<<"Runs:";
		cout<<" requested="<<g->GetRunRequested();
		cout<<" found="<<g->GetRunFound();
		cout<<" Validity range="<<g->GetRunMin()<<"-"<<g->GetRunMax();
		cout<<endl;
		cout<<"URL: "<<g->GetURL()<<endl;
		cout<<"context: "<<g->GetContext()<<endl;
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
	
	// For stuck threads, we may need to cancel them at an arbitrary execution
	// point so we set our cancel type to PTHREAD_CANCEL_ASYNCHRONOUS.
	int oldtype;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	// Create JEventLoop object. He automatically registers himself
	// with the JApplication object. 
	JEventLoop *eventLoop = new JEventLoop((JApplication*)arg);
	
	// Add a cleanup routine to this thread so he can automatically
	// de-register himself when the thread exits for any reason
	pthread_cleanup_push(CleanupThread, (void*)eventLoop);

	// Loop over events until done. Catch any jerror_t's thrown
	try{
		eventLoop->RefreshProcessorListFromJApplication(); // make sure we're up-to-date
		eventLoop->Loop();
		eventLoop->GetJApplication()->Lock();
		cout<<"Thread 0x"<<hex<<(unsigned long)pthread_self()<<dec<<" completed gracefully"<<endl;
		eventLoop->GetJApplication()->Unlock();
	}catch(JException *exception){
		if(exception)delete exception;
		cerr<<__FILE__<<":"<<__LINE__<<" EXCEPTION caught for thread "<<pthread_self()<<endl;
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
	if(arg!=NULL){
		JEventLoop *loop = (JEventLoop*)arg;
		delete loop; // This will cause the destructor to de-register the JEventLoop
	}
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
	
	// At this point, we may have added some event processors through plugins
	// that were not present before we were called. Any JEventLoop objects
	// that exist would not have these in their list of prcessors. Refresh
	// the lists for all event loops
	for(unsigned int i=0; i<loops.size(); i++)loops[i]->RefreshProcessorListFromJApplication();

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
	/// Begin processing events. This will launch the specified number of threads
	/// and start them processing events. This method will return only when 
	/// all events have been processed, or processing is stopped early by the
	/// user.

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
	usleep(100000); // give time for above message to print before messages from threads interfere.
	for(int i=0; i<Nthreads; i++){
		pthread_t thr;
		pthread_create(&thr, NULL, LaunchThread, this);
		cout<<".";cout.flush();
	}
	cout<<endl;
	
	// Get the max time for a thread to be inactive before being deleting
	double THREAD_TIMEOUT=8.0;
	jparms->SetDefaultParameter("THREAD_TIMEOUT", THREAD_TIMEOUT);
	
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
			int delta_NEvents = NEvents - GetEventBufferSize() - last_NEvents;
			avg_NEvents += delta_NEvents>0 ? delta_NEvents:0;
			avg_time += sleep_time;
			rate_instantaneous = (double)delta_NEvents/sleep_time;
			rate_average = (double)avg_NEvents/avg_time;
		}else{
			cout<<__FILE__<<":"<<__LINE__<<" didn't sleep full "<<sleep_time<<" seconds!"<<endl;
		}
		last_NEvents = NEvents - GetEventBufferSize();
		
		// If show_ticker is set, then update the screen with the rate(s)
		if(show_ticker && loops.size()>0)PrintRate();
		
		if(SIGINT_RECEIVED)Quit();
		if(SIGINT_RECEIVED>=3)break;
		
		// Here we lock the app mutex before looping over the heartbeats since a thread
		// could finish at any time, changing the heartbeats vector
		pthread_mutex_lock(&app_mutex);
		
		// Add time slept to all heartbeats
		double rem_time = (double)rem.tv_sec + (1.0E-9)*(double)rem.tv_nsec;
		double slept_time = sleep_time - rem_time;
		for(unsigned int i=0;i<heartbeats.size();i++){
			double *hb = heartbeats[i];
			*hb += slept_time;
			if(monitor_heartbeat && (*hb > (THREAD_TIMEOUT-1.0)+sleep_time)){
				// Thread hasn't done anything for more than THREAD_TIMEOUT seconds. 
				// Remove it from monitoring lists.
				JEventLoop *loop = *(loops.begin()+i);
				JEvent &event = loop->GetJEvent();
				cerr<<" Thread "<<i<<" hasn't responded in "<<*hb<<" seconds.";
				cerr<<" (run:event="<<event.GetRunNumber()<<":"<<event.GetEventNumber()<<")";
				cerr<<" Cancelling ..."<<endl;
				
				// At this point, we need to kill the stalled thread. We do this by
				// calling pthread_cancel which will subsequently destroy the JEventLoop
				// in the cleanup routine (CleanupThread()) which will remove the
				// thread and all its pieces (heartbeat, etc..) from the JApplication
				// by calling RemoveJEventLoop from the JEventLoop destructor (yeah,
				// I know, it's complicated). Note this will only schedule the stalled
				// thread to call CleanupThread so it may not happen right away. What's
				// more, the call will get blocked when it tries to lock the app_mutex
				// which we currently have locked.
				pthread_cancel(threads[i]);

				// Launch a new thread to take his place, but only if we're not trying to quit
				if(!SIGINT_RECEIVED){
					cerr<<" Launching new thread ..."<<endl;
					pthread_t thr;
					pthread_create(&thr, NULL, LaunchThread, this);
					
					// We need to wait for the new thread to create a new JEventLoop
					// If we don't wait for this here, then control may fall to the
					// end of the while loop that checks loops.size() before the new
					// loop is added, causing the program to finish prematurely.
					for(int j=0; j<40; j++){
						struct timespec req, rem;
						req.tv_nsec = (int)0.100E9; // set to 100 milliseconds
						req.tv_sec = 0;
						rem.tv_sec = rem.tv_nsec = 0;
						pthread_mutex_unlock(&app_mutex); // Unlock the mutex
						nanosleep(&req, &rem);
						pthread_mutex_lock(&app_mutex); // Re-lock the mutex

						// Our new thread will be in "threads" once it's up and running
						if(find(threads.begin(), threads.end(), thr)!=threads.end())break;
					}
				}

				// Quit this loop so we can re-enter it fresh since the thread list was changed
				break;
			}
		}
		
		// We're done with the heartbeats etc. vectors for now. Unlock the mutex.
		pthread_mutex_unlock(&app_mutex);


		// When a JEventLoop runs out of events, it removes itself from
		// the list before returning from the thread.
	}while(loops.size() > 0);
	
	// Only be nice about exiting if the user wasn't insistent
	if(SIGINT_RECEIVED<3){	
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

			// Look for a FiniPlugin symbol and execute if found
			FiniPlugin_t *plugin = (FiniPlugin_t*)dlsym(sohandles[i], "FiniPlugin");
			if(plugin){
				cout<<"Finalizing plugin ..."<<endl;
				(*plugin)(this);
			}
			
			// Close shared object
			dlclose(sohandles[i]);
		}
	}else{
		cout<<"Exiting hard due to catching 3 or more SIGINTs ..."<<endl;
	}
	
	cout<<" "<<NEvents<<" events processed ";
	cout<<" ("<<NEvents_read<<" events read) ";
	cout<<"Average rate: "<<Val2StringWithPrefix(rate_average)<<"Hz"<<endl;

	if(SIGINT_RECEIVED>=3)exit(-1);

	return NOERROR;
}

//---------------------------------
// Fini
//---------------------------------
jerror_t JApplication::Fini(void)
{
	/// Close out at the end of event processing. All <i>erun()</i> routines
	/// and <i>fini()</i> routines are called as necessary. All JEventSources
	/// are deleted and the final report is printed, if specified.

	Lock();
	if(fini_called){Unlock(); return NOERROR;}
	fini_called=true;
	Unlock();
	
	// Print final resource report
	if(print_resource_report)PrintResourceReport();
	
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
	
	// Tell event buffer thread to quit (if he hasn't already)
	for(int i=0; i<10; i++){
		if(!event_buffer_filling)break;
		stop_event_buffer = true;
		pthread_cond_signal(&event_buffer_cond);
		usleep(100000);
	}
	
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
	/// Resume event processing for all threads.
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
	/// Quit the application. This will invoke the Quit() method of all
	/// JEventLoops. This does not force a runaway thread to be killed and
	/// will only cause the program to quit if all JEventLoops quit cleanly.
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
		return RESOURCE_UNAVAILABLE;
	}
	
	// Look for an InitPlugin symbol
	InitPlugin_t *plugin = (InitPlugin_t*)dlsym(handle, "InitPlugin");
	if(plugin){
		cout<<"Initializing plugin \""<<soname<<"\" ..."<<endl;
		(*plugin)(this);
		sohandles.push_back(handle);
	}else{
		dlclose(handle);
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
		bool found_plugin=false;
		for(unsigned int i=0; i< pluginPaths.size(); i++){
			string fullpath = pluginPaths[i] + "/" + plugin;
			ifstream f(fullpath.c_str());
			if(f.is_open()){
				f.close();
				if(RegisterSharedObject(fullpath.c_str())==NOERROR)found_plugin=true;
				break;
			}
			if(printPaths) cout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
			
			if(fullpath[0] != '/')continue;
			fullpath = pluginPaths[i] + "/" + plugins[j] + "/" + plugin;
			f.open(fullpath.c_str());
			if(f.is_open()){
				f.close();
				if(RegisterSharedObject(fullpath.c_str())==NOERROR)found_plugin=true;
				break;
			}
			if(printPaths) cout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
		}
		
		// If we didn't find the plugin, then complain and quit
		if(!found_plugin){
			Lock();
			cerr<<endl<<"***ERROR : Couldn't find plugin \""<<plugins[j]<<"\"!***"<<endl;
			cerr<<"***        To see paths checked, set JANA_PRINT_PLUGIN_PATHS env. var. and re-run"<<endl;
			Unlock();
			exit(-1);
		}
	}

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
	
	// Lock mutex and add to master list
	Lock();
	Nfactory_calls[pthread_self()] = calls;
	Nfactory_gencalls[pthread_self()] = gencalls;
	Unlock();

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
	
	// Print column headers
	string header1(80,' ');
	string header2(80,' ');
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
	string hr(80,'-');
	cout<<hr<<endl;
	
	// Loop over nametags
	for(unsigned int i=0; i<nametag.size(); i++){
		string &name = nametag[i];
		string line(80,' ');
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
// SignalThreads
//---------------------------------
void JApplication::SignalThreads(int signo)
{
	// Send signal "signo" to all processing threads.
	for(unsigned int i=0; i<threads.size(); i++){
		pthread_kill(threads[i], signo);
	}
}
