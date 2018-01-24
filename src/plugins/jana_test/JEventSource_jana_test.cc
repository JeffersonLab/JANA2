// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "JEventSource_jana_test.h"
#include "JEvent_test.h"

//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(const char* source_name) : JEventSource(source_name, japp)
{
	// Allocate pool of JEvent objects
	mEventsFromFile.reserve(10000);
	std::vector<std::shared_ptr<JEvent_test>> mEventsFromFile; //simulates a file

	for(std::size_t i = 0; i < mEventsFromFile.capacity(); i++)
		mEventsFromFile.emplace_back(new JEvent_test());
}

//----------------
// Destructor
//----------------
JEventSource_jana_test::~JEventSource_jana_test()
{
}

//----------------
// GetEvent
//----------------
std::pair<std::shared_ptr<JEvent>, JEventSource::RETURN_STATUS> JEventSource_jana_test::GetEvent(void)
{
	/// Read an event (or possibly block of events) from the source return it.
	
	// If an event was successfully read in, return kSUCCESS. If there
	// are no more events in the source to read, return kNO_MORE_EVENTS.
	// If the source has no events at the moment, but may later (e.g.
	// a live stream) then return kTRY_AGAIN;

	if(mEventsFromFile.empty())
		return std::make_pair(std::shared_ptr<JEvent>(nullptr), JEventSource::RETURN_STATUS::kNO_MORE_EVENTS);

	auto event = std::move(mEventsFromFile[mEventsFromFile.size() - 1]);
	mEventsFromFile.pop_back();
	
	event->SetEventSource(this);
//	event.SetEventNumber(++Nevents_read);
//	event.SetRunNumber(1234);
//	event.SetRef(NULL);

	
	return std::make_pair(std::static_pointer_cast<JEvent>(event), JEventSource::RETURN_STATUS::kSUCCESS);
}

