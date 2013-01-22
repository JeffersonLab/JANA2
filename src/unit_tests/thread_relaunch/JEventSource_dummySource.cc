// $Id$
//
//    File: JEventSource_dummySource.cc
// Created: Tue Jan 22 11:33:11 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//


#include "JEventSource_dummySource.h"
using namespace jana;

//----------------
// Constructor
//----------------
JEventSource_dummySource::JEventSource_dummySource(const char* source_name):JEventSource(source_name)
{
	// open event source (e.g. file) here
}

//----------------
// Destructor
//----------------
JEventSource_dummySource::~JEventSource_dummySource()
{
	// close event source here
}

//----------------
// GetEvent
//----------------
jerror_t JEventSource_dummySource::GetEvent(JEvent &event)
{
	// Read an event from the source and copy the vital info into
	// the JEvent structure. The "Ref" of the JEventSource class
	// can be used to hold a pointer to an arbitrary object, though
	// you'll need to cast it to the correct pointer type 
	// in the GetObjects method.
	event.SetJEventSource(this);
	event.SetEventNumber(++Nevents_read);
	event.SetRunNumber(1234);
	event.SetRef(NULL);

	// If an event was sucessfully read in, return NOERROR. Otherwise,
	// return NO_MORE_EVENTS_IN_SOURCE. By way of example, this
	// will return NO_MORE_EVENTS_IN_SOURCE after 100k events.
	if(Nevents_read>=100000)return NO_MORE_EVENTS_IN_SOURCE;
	
	return NOERROR;
}

//----------------
// FreeEvent
//----------------
void JEventSource_dummySource::FreeEvent(JEvent &event)
{
	// If memory was allocated in GetEvent, it can be freed here. This
	// would typically be done by using event.GetRef() and casting the
	// returned void* into the proper pointer type so it can be deleted.
}

//----------------
// GetObjects
//----------------
jerror_t JEventSource_dummySource::GetObjects(JEvent &event, JFactory_base *factory)
{
	// This callback is called to extract objects of a specific type from
	// an event and store them in the factory pointed to by JFactory_base.
	// The data type desired can be obtained via factory->GetDataClassName()
	// and the tag via factory->Tag().
	//
	// If the object is not one of a type this source can provide, then
	// it should return OBJECT_NOT_AVAILABLE. Otherwise, it should return
	// NOERROR;
	
	// We must have a factory to hold the data
	if(!factory)throw RESOURCE_UNAVAILABLE;

	// Get name of data class we're trying to extract and the factory tag
	string dataClassName = factory->GetDataClassName();
	string tag = factory->Tag();
	
	// Example for providing objects of type XXX
	//
	// // Get pointer to object of type MyEvent (this is optional) 
	// MyEvent *myevent = (MyEvent*)event.GetRef();
	//
	// if(dataClassName == "XXX"){
	//
	//	 // Make objects of type XXX storing them in a vector
	//   vector<XXX*> xxx_objs;
	//	 ...
	//
	//	 JFactory<XXX> *fac = dynamic_cast<JFactory<XXX>*>(factory);
	//	 if(fac)fac->CopyTo(xxx_objs);
	//	
	//	 return NOERROR;
	// }
	
	// For all other object types, just return OBJECT_NOT_AVAILABLE to indicate
	// we can't provide this type of object
	return OBJECT_NOT_AVAILABLE;
}

