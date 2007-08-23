// $Id$
//
// Author: David Lawrence  July 14, 2006
//
//
// JEventSourceTest methods
//


#include <iostream>
#include <iomanip>
#include <cmath>
using namespace std;

#include <JANA/JApplication.h>
#include <JANA/JFactory_base.h>
#include <JANA/JFactory.h>
#include <JANA/JEventLoop.h>
#include <JANA/JEvent.h>

#include "JEventSourceTest.h"
#include "JEventSourceTestGenerator.h"
#include "JFactoryGeneratorTest.h"
#include "JEventProcessorTest.h"

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddEventSourceGenerator(new JEventSourceTestGenerator());
	app->AddFactoryGenerator(new JFactoryGeneratorTest());
	app->AddProcessor(new JEventProcessorTest());
	
	cout<<endl;
	cout<<"Initialized speed_test plugin."<<endl;
	cout<<"For this to work you need to give at least one dummy source."<<endl;
	cout<<"e.g."<<endl;
	cout<<"        jana --plugin=jpseed_test dummy"<<endl;
	cout<<endl;
	cout<<"There are built-in delays in both the processor and source"<<endl;
	cout<<"that can be adjusted to more realistically test the system."<<endl;
	cout<<"These are controlled by configuration parameters on the command"<<endl;
	cout<<"line. To control the sleep time spent in the event source, use:"<<endl;
	cout<<endl;
	cout<<"    -PMAX_IO_RATE_HZ=100"<<endl;
	cout<<endl;
	cout<<"this will cause the source the sleep for 1/100th of a second for"<<endl;
	cout<<"each event."<<endl;
	cout<<endl;
	cout<<"To adjust the time spent in the processor, use:"<<endl;
	cout<<endl;
	cout<<"    -PGOVENOR_ITERATIONS=1000"<<endl;
	cout<<endl;
	cout<<"this will cause the \"number-crunching\" loop to iterate 1000"<<endl;
	cout<<"times so that CPU cycles are chewed up in the processor itself."<<endl;
	cout<<endl;
}
} // "C"

//----------------
// Constructor
//----------------
JEventSourceTest::JEventSourceTest(const char* source_name):JEventSource(source_name)
{
	/// Constructor for JEventSourceTest object
	cout<<"Opening fake event generator. No actual events will be read in"<<endl;
	
	MAX_IO_RATE_HZ = 100;
	
	gPARMS->SetDefaultParameter("MAX_IO_RATE_HZ",MAX_IO_RATE_HZ);
}

//----------------
// Destructor
//----------------
JEventSourceTest::~JEventSourceTest()
{
	// Close file and set pointers to NULL
	cout<<"Closing fake event generator."<<endl;
}

//----------------
// GetEvent
//----------------
jerror_t JEventSourceTest::GetEvent(JEvent &event)
{
	/// Implementation of JEventSource::GetEvent function
	
	int event_number = ++Nevents_read;
	int run_number = 1234;

	// Copy the reference info into the JEvent object
	event.SetJEventSource(this);
	event.SetEventNumber(event_number);
	event.SetRunNumber(run_number);
	event.SetRef(NULL);

	// Sleep to emulate I/O delay. This will not give an entirely accurate
	// result since it gives up the delay time for use by other threads.
	// It will at least limit the overall I/O rate which is more realistic
	// than no delay.
	if(MAX_IO_RATE_HZ>0){
		struct timespec rqtp, rmtp;
		double delay = 1.0/(double)MAX_IO_RATE_HZ;
		rqtp.tv_sec = (unsigned long)floor(delay);
		rqtp.tv_nsec = (unsigned long)((delay-floor(delay))*1.0E9);
		nanosleep(&rqtp, &rmtp);
	}

	return NOERROR;
}

//----------------
// FreeEvent
//----------------
void JEventSourceTest::FreeEvent(JEvent &event)
{
	// Nothing to be done here.
}

//----------------
// GetObjects
//----------------
jerror_t JEventSourceTest::GetObjects(JEvent &event, JFactory_base *factory)
{
	/// This gets called through the virtual method of the
	/// JEventSource base class. It creates the objects of the type
	/// which factory is based.

	// We must have a factory to hold the data
	if(!factory)throw RESOURCE_UNAVAILABLE;

	// Get name of data class we're trying to extract
	string dataClassName = factory->dataClassName();
	
	// Just return. The _data vector should already be reset to have zero objects
	return OBJECT_NOT_AVAILABLE;
}


