// $Id$
//
// Author: David Lawrence  July 14, 2006
//
//
// JEventSourceEVIO methods
//


#include <iostream>
#include <iomanip>
using namespace std;

#include <JANA/JApplication.h>
#include <JANA/JFactory_base.h>
#include <JANA/JFactory.h>
#include <JANA/JEventLoop.h>
#include <JANA/JEvent.h>

#include "JEventSourceEVIO.h"
#include "JEventSourceEVIOGenerator.h"

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	app->AddEventSourceGenerator(new JEventSourceEVIOGenerator());
}
} // "C"

//----------------
// Constructor
//----------------
JEventSourceEVIO::JEventSourceEVIO(const char* source_name):JEventSource(source_name)
{
	/// Constructor for JEventSourceEVIO object
	try{
		evioFile = new evioFileChannel(source_name, "r");
		evioFile->open();
	}catch(evioException *e){
		evioFile = NULL;
		cerr<<e->toString()<<endl;
	}
}

//----------------
// Destructor
//----------------
JEventSourceEVIO::~JEventSourceEVIO()
{
	// Close file and set pointers to NULL
	if(evioFile){
		evioFile->close();
		delete evioFile;
		evioFile = NULL;
	}
}

//----------------
// GetEvent
//----------------
jerror_t JEventSourceEVIO::GetEvent(JEvent &event)
{
	/// Implementation of JEventSource::GetEvent function
	
	// Make sure we have a non-NULL pointer
	if(!evioFile){
		return EVENT_SOURCE_NOT_OPEN;
	}
	
	// Read in an event. If error, notify caller
	if(! evioFile->read()){
		return NO_MORE_EVENTS_IN_SOURCE;
	}
	++Nevents_read;
	
	// Create a DOM tree of the event
	evioDOMTree *tree = new evioDOMTree(*evioFile);

	int event_number = Nevents_read;
	int run_number = -1;

	// Copy the reference info into the JEvent object
	event.SetJEventSource(this);
	event.SetEventNumber(event_number);
	event.SetRunNumber(run_number);
	event.SetRef(tree);

	return NOERROR;
}

//----------------
// FreeEvent
//----------------
void JEventSourceEVIO::FreeEvent(JEvent &event)
{
	evioDOMTree *tree = (evioDOMTree*)event.GetRef();
	delete tree;
}

//----------------
// GetObjects
//----------------
jerror_t JEventSourceEVIO::GetObjects(JEvent &event, JFactory_base *factory)
{
	/// This gets called through the virtual method of the
	/// JEventSource base class. It creates the objects of the type
	/// which factory is based. It uses the s_HDDM_t object
	/// kept in the ref field of the JEvent object passed.

	// We must have a factory to hold the data
	if(!factory)throw RESOURCE_UNAVAILABLE;
	
	// HDDM doesn't support tagged factories
	if(strcmp(factory->Tag(), ""))return OBJECT_NOT_AVAILABLE;
	
	// The ref field of the JEvent is just the s_HDDM_t pointer.
//	s_HDDM_t *my_hddm_s = (s_HDDM_t*)event.GetRef();
//	if(!my_hddm_s)throw RESOURCE_UNAVAILABLE;
	
	// Get name of data class we're trying to extract
	string dataClassName = factory->GetDataClassName();
	
	// At this point, we have two options. One is that we have the
	// source only provide one type of object: an evioDOMTree. This
	// means it is up to the other factories to extract their
	// info from the evioDOMTree. The other is to extract the
	// info from the evioDOMTree right here and create the objects
	// as asked for. The advantages of the first method are that
	// it is simple to implement here and is very flexible if
	// one adds objects to the evio files. The advantage of the
	// second method is that the evioDOMTree which is specific
	// to evio files (and streams) is contained completely in
	// this source. This essentially forces the factories to stay
	// agnostic about the event source type.
	
	// For the sake of simplicity here, we use the former method.
	
	if(dataClassName =="evioDOMTree"){
		vector<evioDOMTree*> trees;
		trees.push_back((evioDOMTree*)event.GetRef());
		
		// Copy into factory
		JFactory<evioDOMTree> *fac = dynamic_cast<JFactory<evioDOMTree>*>(factory);
		if(fac){
			fac->CopyTo(trees);
		}else{
			cerr<<__FILE__<<":"<<__LINE__<<" Hmmm... factory passed does not seem to be a JFactory<evioDOMTree>"<<endl;
			return UNKNOWN_ERROR;
		}
		
		return NOERROR;
	}
	
	return OBJECT_NOT_AVAILABLE;
}


