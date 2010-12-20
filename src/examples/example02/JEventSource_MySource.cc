// $Id$
//
//    File: JEventSource_MySource.cc
// Created: Mon Dec 20 08:18:56 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//


#include "JEventSource_MySource.h"
using namespace jana;

#include "RawHit.h"

//----------------
// Constructor
//----------------
JEventSource_MySource::JEventSource_MySource(const char* source_name):JEventSource(source_name)
{
	// open event source (e.g. file) here
}

//----------------
// Destructor
//----------------
JEventSource_MySource::~JEventSource_MySource()
{
	// close event source here
}

//----------------
// GetEvent
//----------------
jerror_t JEventSource_MySource::GetEvent(JEvent &event)
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
	// will return NO_MORE_EVENTS_IN_SOURCE after 10k events.
	if(Nevents_read>10000)return NO_MORE_EVENTS_IN_SOURCE;
	
	return NOERROR;
}

//----------------
// FreeEvent
//----------------
void JEventSource_MySource::FreeEvent(JEvent &event)
{
	// If memory was allocated in GetEvent, it can be freed here. This
	// would typically be done by using event.GetRef() and casting the
	// returned void* into the proper pointer type so it can be deleted.
}

//----------------
// GetObjects
//----------------
jerror_t JEventSource_MySource::GetObjects(JEvent &event, JFactory_base *factory)
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
	
	// RawHit objects with no tag were requested.
	if(dataClassName == "RawHit" && tag==""){
		int Nhits = random()%100; // randomly create between 0 and 99 RawHit objects
		vector<RawHit*> hits;
		for(int i=0; i<Nhits; i++){
			RawHit *hit = new RawHit();
			
			// You would normally fill these with values read from the
			// file/socket. For this example though, we just put in
			// random values.
			hit->crate = 1 + random()%50;
			hit->slot = 1 + random()%24;
			hit->channel = random()%32;
			hit->adc = random()%4096;
			
			hits.push_back(hit);
		}
		
		// Cast the pointer to the factory to one that actually has access
		// to the internal container of the appropriate type. Copy the
		// pointers if successful.
		JFactory<RawHit> *fac = dynamic_cast<JFactory<RawHit>*>(factory);
		if(fac)fac->CopyTo(hits);
		
		return NOERROR; // indicate to framework we know about these objects
	}
	
	// For all other object types, just return OBJECT_NOT_AVAILABLE to indicate
	// we can't provide this type of object
	return OBJECT_NOT_AVAILABLE;
}

