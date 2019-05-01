// $Id$
//
//    File: JEventxample2.cc
// Created: Thu Apr 26 22:07:34 EDT 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.5.0 x86_64)
//

#include <memory>
#include <utility>
#include <random>

//#include <JANA/JEventSource.h>
//#include <JANA/JFactory.h>
//
//#include <JANA/JApplication.h>
//#include <JANA/JEventSourceGenerator.h>
//#include <JANA/JEventSourceManager.h>
//#include <JANA/JQueue.h>
//#include <JANA/JEvent.h>
//#include <JANA/JEventSource.h>

#include "JEventSource_example2.h"
#include "MyEvent.h"


//-------------------------------------------------------------------------
// Open
//
// The Open method is called before events are read from this source. You
// could open the file in the constructor, but it is recommended to wait
// until Open is called. This is because JEventSource objects are made at
// the beginning of the job, though not all files will be read right away.
//-------------------------------------------------------------------------
void JEventSource_example2::Open(void)
{

}

//-------------------------------------------------------------------------
// GetEvent
//
// This method is called to read in a single "event"
//-------------------------------------------------------------------------
void JEventSource_example2::GetEvent(std::shared_ptr<JEvent> jevent)
{
	// Throw exception if we have exhausted the source of events
	static size_t Nevents = 0; // by way of example, just count 1000000 events
	if( ++Nevents > 1000000 ) throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
	
	// Create a JEvent object and fill in important info. It is expected that
	// the most common operation will be to read in a buffer and store a pointer
	// to it in the event object. You should not spend much effort here processing
	// the data since this method is called serially. Defer as much as possible
	// to the GetObjects method below which may be called in parallel.

	// jevent->Insert(...); // buffer

}

//-------------------------------------------------------------------------
// GetObjects
//-------------------------------------------------------------------------
bool JEventSource_example2::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory)
{
	// This will get called for every type of object requested for the event
	// to see if the objects are available from the source. In this example,
	// the source only provides one type of object: MyHit. If this is not the
	// type being requested, then return "false" immediately.
	if( aFactory->GetName() != "MyHit" ) return false;

	// Here, one would normally get a buffer stored in the JEvent and parse it
	// to make the low-level objects (e.g. MyHit, ...). For this example though,
	// we just generate some hit objects using random numbers.
	static thread_local std::default_random_engine generator;

	// Determine how many MyHit objects this hit will have
	std::normal_distribution<double> distribution(50.0,10.0);
	size_t Nhits = 0;
	while(Nhits<10) Nhits = distribution(generator);

	// Create MyHit objects
	vector<MyHit*> hits;
	for(size_t i=0; i<Nhits; i++){
		double x = distribution(generator)/10.0;
		double E = distribution(generator)*2.0;
		double t = distribution(generator)/3.0;
		hits.push_back( new MyHit( x, E, t) );
	}
	
	// Copy pointers into factory, transferring ownership. In this case, the
	// factory is just used as a container and has no algorithm. See the
	// MyCluster class for an example of a factory with an algorithm
	aFactory->Set( hits );

	// Return true to indicate that this is a type of object the source can provide.
	return true;
}

