// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "JEventSource_jana_test.h"
#include "JEvent_test.h"

#include <iostream>

//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(const char* source_name) : JEventSource(source_name, japp)
{
	// Allocate pool of JEvent objects
	mNumEventsToGenerate = 20000;
}

//----------------
// Destructor
//----------------
JEventSource_jana_test::~JEventSource_jana_test()
{
	//Close the file/stream handle
}

//----------------
// GetEvent
//----------------
std::pair<std::shared_ptr<const JEvent>, JEventSource::RETURN_STATUS> JEventSource_jana_test::GetEvent(void)
{
	/// Read an event (or possibly block of events) from the source return it.
	
	// If an event was successfully read in, return kSUCCESS. If there
	// are no more events in the source to read, return kNO_MORE_EVENTS.
	// If the source has no events at the moment, but may later (e.g.
	// a live stream) then return kTRY_AGAIN;

	if(mNumEventsToGenerate == mNumEventsGenerated)
		return std::make_pair(std::shared_ptr<JEvent>(nullptr), JEventSource::RETURN_STATUS::kNO_MORE_EVENTS);

	auto sEvent = mEventPool.Get_SharedResource(mApplication);
	mNumEventsGenerated++;
	
	sEvent->SetEventSource(this);
	sEvent->SetEventNumber(mNumEventsGenerated);
	sEvent->SetRunNumber(1234);
//	sEvent->SetRef(nullptr);
	
	return std::make_pair(std::static_pointer_cast<JEvent>(sEvent), JEventSource::RETURN_STATUS::kSUCCESS);
}

//----------------
// Open
//----------------
void JEventSource_jana_test::Open(void)
{
	//Open the file/stream handle
}

//----------------
// GetObjects
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactoryBase* aFactory)
{
	//Get objects of the specified type, and set them in the factory
	//If this type is not supplied by the file, return false. Else return true.
	auto sTypeIndex = aFactory->GetObjectType();
	return false;
}
