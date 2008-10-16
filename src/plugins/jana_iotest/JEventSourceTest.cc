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

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddEventSourceGenerator(new JEventSourceTestGenerator());
	
	cout<<endl;
	cout<<"Initialized io_speed_test plugin."<<endl;
	cout<<"For this to work you need to give at least one actual file."<<endl;
	cout<<"e.g."<<endl;
	cout<<"        jana --plugin=jana_iotest filename"<<endl;
	cout<<endl;
	cout<<"The file will be opened and blocks of data read from it"<<endl;
	cout<<"which are then discarded. This is does not implement any"<<endl;
	cout<<"Event processors and is only meant to test the I/O read speed"<<endl;
	cout<<"using JANA. To set the read block size do add this:"<<endl;
	cout<<endl;
	cout<<"    -PREAD_BLOCK_SIZE=1024"<<endl;
	cout<<endl;
	cout<<"this will cause the source the reads to be done in 1kB(=1024 byte) blocks."<<endl;
	cout<<endl;
	cout<<endl;
}
} // "C"

//----------------
// Constructor
//----------------
JEventSourceTest::JEventSourceTest(const char* source_name):JEventSource(source_name)
{
	/// Constructor for JEventSourceTest object
	cout<<"Opening file \""<<source_name<<"\""<<endl;
	
	READ_BLOCK_SIZE = 837;
	
	gPARMS->SetDefaultParameter("READ_BLOCK_SIZE",READ_BLOCK_SIZE);
	
	buff = new char[READ_BLOCK_SIZE];
	
	ifs = new ifstream(source_name);
}

//----------------
// Destructor
//----------------
JEventSourceTest::~JEventSourceTest()
{
	// Close file and set pointers to NULL
	cout<<"Closing file \""<<source_name<<"\""<<endl;
	
	delete[] buff;
	
	delete ifs;
}

//----------------
// GetEvent
//----------------
jerror_t JEventSourceTest::GetEvent(JEvent &event)
{
	/// Implementation of JEventSource::GetEvent function
	
	int event_number = ++Nevents_read;
	int run_number = 1234;

	// Read in block of events
	ifs->read(buff, READ_BLOCK_SIZE);
	if(ifs->eof())return NO_MORE_EVENTS_IN_SOURCE;

	// Copy the reference info into the JEvent object
	event.SetJEventSource(this);
	event.SetEventNumber(event_number);
	event.SetRunNumber(run_number);
	event.SetRef(buff);

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
	string dataClassName = factory->GetDataClassName();
	
	// Just return. The _data vector should already be reset to have zero objects
	return OBJECT_NOT_AVAILABLE;
}


