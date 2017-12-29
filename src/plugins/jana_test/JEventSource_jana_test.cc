// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//


#include "JEventSource_jana_test.h"
using namespace std;



//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(const char* source_name):JEventSource(source_name)
{
	// Create 3 JQueues in the JApplication
	_queue_primary   = new JQueuePrimary();
	_queue_secondary = new JQueueSecondary();
	_queue_final     = new JQueueFinal();

	japp->AddJQueue( _queue_primary );
	japp->AddJQueue( _queue_secondary );
	japp->AddJQueue( _queue_final );
}

//----------------
// Destructor
//----------------
JEventSource_jana_test::~JEventSource_jana_test()
{
	// close event source here
}

//----------------
// GetEvent
//----------------
JEventSource::RETURN_STATUS JEventSource_jana_test::GetEvent(void)
{
	/// Read an event (or possibly block of events) from the source and 
	/// place it (or them) into the appropriate JQueue's. This must 
	/// provide a JEvent of the appropriate type for the JQueue.
	
	// We just use the base class here. In a real implementation, this
	// would be a subclass of JEvent.
	JEvent *event = new JEvent();
	_queue_primary->AddToQueue( event );
	
//	event.SetJEventSource(this);
//	event.SetEventNumber(++Nevents_read);
//	event.SetRunNumber(1234);
//	event.SetRef(NULL);

	// If an event was sucessfully read in, return kSUCCESS. If there
	// are no more events in the source to read, return kNO_MORE_EVENTS.
	// If the source has no events at the moment, but may later (e.g.
	// a live stream) then return kTRY_AGAIN;
//	if(Nevents_read>=100000)return NO_MORE_EVENTS_IN_SOURCE;
	
	return kSUCCESS;
}

//----------------
// FreeEvent
//----------------
void JEventSource_jana_test::FreeEvent(JEvent &event)
{
	// If memory was allocated in GetEvent, it can be freed here. This
	// would typically be done by using event.GetRef() and casting the
	// returned void* into the proper pointer type so it can be deleted.
	
	delete &event;
}

//----------------
// GetObjects
//----------------
void JEventSource_jana_test::GetObjects(JEvent &event, JFactory *factory)
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
	if(!factory)throw JException("GetObjects called with NULL factory pointer");

	// Get name of data class we're trying to extract and the factory tag
//	string dataClassName = factory->GetDataClassName();
//	string tag = factory->Tag();
	
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
	
}

