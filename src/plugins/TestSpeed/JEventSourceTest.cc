// $Id$
//
// Author: David Lawrence  July 14, 2006
//
//
// JEventSourceTest methods
//


#include <iostream>
#include <iomanip>
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
	cout<<"Also note that the average processing rate will start off low"<<endl;
	cout<<"due to the initial event buffer filling. The program should be"<<endl;
	cout<<"allowed to run for at leat 1 minute in order for it to reach"<<endl;
	cout<<"a steady state."<<endl;
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


