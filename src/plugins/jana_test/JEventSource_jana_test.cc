// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//


#include "JEventSource_jana_test.h"
#include "JEvent_test.h"

using namespace std;



//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(const char* source_name):JEventSource(source_name),_jevent_pool("pool")
{
	// Create 3 JQueues in the JApplication
	_queue_primary   = new JQueuePrimary();
	_queue_secondary = new JQueueSecondary();
	_queue_final     = new JQueueFinal();

	japp->AddJQueue( _queue_primary );
	japp->AddJQueue( _queue_secondary );
	japp->AddJQueue( _queue_final );
	
	// Allocate pool of JEvent objects
	for(int i=0;i<100;i++) _jevent_pool.AddToQueue( new JEvent_test( this ) );
}

//----------------
// Destructor
//----------------
JEventSource_jana_test::~JEventSource_jana_test()
{
	// Delete events in pool
	JEvent *event;
	while( (event=_jevent_pool.GetEvent()) ) delete event;
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
	JEvent *event = _jevent_pool.GetEvent();
	if( event ) _queue_primary->AddToQueue( event );
	
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
// ReturnToPool
//----------------
void JEventSource_jana_test::ReturnToPool(JEvent *event)
{
	/// Return the JEvent to the pool
	_jevent_pool.AddToQueue( event );
}



