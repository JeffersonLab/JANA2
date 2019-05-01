// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "JEventSource_jana_test.h"
#include "JEvent_test.h"
#include "JSourceFactoryGenerator.h"
#include "JEventSourceGeneratorT.h"
#include "JTask.h"
#include "JLogger.h"
#include "JQueueWithLock.h"
#include "JQueueWithBarriers.h"
#include "TextEventRecord.h"

thread_local std::mt19937 gRandomGenerator;


// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JEventSource_jana_test>::CheckOpenable(std::string source) { return 1.0E-6; }


//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(string source_name, JApplication *japp) : JEventSource(source_name, japp)
{
	mNumEventsToGenerate = 20000;
	japp->GetJParameterManager()->SetDefaultParameter(
		"NEVENTS", 
		mNumEventsToGenerate, 
		"Number of events for fake event source to generate");

	japp->GetJParameterManager()->SetDefaultParameter(
		"JTEST:INCLUDE_BARRIER_EVENTS", 
		mIncludeBarriers, 
		"Include barrier events");

	//Seed random number generator //not ideal!
	auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	mRandomGenerator.seed(sTime);

	//Make factory generator that will make factories for all types provided by the event source
	//This is necessary because the JFactorySet needs all factories ahead of time
	//Make sure that all types are listed as template arguments here!!
	mFactoryGenerator = new JSourceFactoryGenerator<TextEventRecord, JSourceObject, JSourceObject2>();

	auto params = mApplication->GetJParameterManager();

	// Event queue: max size of 200, keep at least 50 buffered
	if (mIncludeBarriers) {
		mEventQueue = new JQueueWithBarriers(params, "Events", 200, 50); 
	}
       	else {
		mEventQueue = new JQueueSimple(params ,"Events", 200, 50);
	}
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
void JEventSource_jana_test::GetEvent(std::shared_ptr<JEvent> aEvent)
{
	/// Read an event (or possibly block of events) from the source return it.
	
	// If an event was successfully read in, return kSUCCESS. If there
	// are no more events in the source to read, return kNO_MORE_EVENTS.
	// If the source has no events at the moment, but may later (e.g.
	// a live stream) then return kTRY_AGAIN;

	if( mNumEventsToGenerate != 0 )
		if(mNumEventsToGenerate == mNumEventsGenerated) throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;

	//These are recycled, so be sure to re-set EVERY member variable
	mNumEventsGenerated++;

	if( mIncludeBarriers ){
		auto sIsBarrierEvent = (mNumEventsGenerated % 100) == 0;
		aEvent->SetIsBarrierEvent(sIsBarrierEvent);
	}
	aEvent->SetEventNumber(mNumEventsGenerated);
	aEvent->SetRunNumber(1234);

	auto ter = new TextEventRecord;
	ter->data = mNumEventsGenerated;
	aEvent->Insert(ter);

}

//----------------
// Open
//----------------
void JEventSource_jana_test::Open(void)
{
	//Open the file/stream handle
}

//---------------------------------
// LockGenerator
//---------------------------------
void JEventSource_jana_test::LockGenerator(void) const
{
	bool sExpected = false;
	while(!mGeneratorLock.compare_exchange_weak(sExpected, true)){
		sExpected = false;
		std::this_thread::sleep_for( std::chrono::nanoseconds(1) );
	}
}

//----------------
// GetObjects
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory)
{
	//Get objects of the specified type, and set them in the factory
	//If this type is not supplied by the file, return false. Else return true.

	//For all of the objects retrievable from the file, we MUST generate factories via our own JFactoryGenerator
	auto sTypeIndex = aFactory->GetObjectType();
	if(sTypeIndex == std::type_index(typeid(JSourceObject)))
		return GetObjects(aEvent, static_cast<JFactoryT<JSourceObject>*>(aFactory));
	else if(sTypeIndex == std::type_index(typeid(JSourceObject2)))
		return GetObjects(aEvent, static_cast<JFactoryT<JSourceObject2>*>(aFactory));
	else
		return false;
}

//----------------
// GetObjects  - JSourceObject
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactoryT<JSourceObject>* aFactory)
{
	if(aFactory->GetTag() != "") return false; //Only default tag here

	// Generate a random # of objects
	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
	auto sNumObjects = sNumObjectsDistribution(gRandomGenerator);
	std::vector<JSourceObject*> sObjects;
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		// Create new JSourceObject
		auto sObject = new JSourceObject(gRandomGenerator(), si);
		sObjects.push_back( sObject );

		// Supply busy work to take time. This would represent
		// the parsing of the input data.
		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(1000, 2000);
		auto sNumRandoms = sNumRandomsDistribution(gRandomGenerator);
		for(std::size_t sj = 0; sj < sNumRandoms; sj++) sObject->AddRandom(gRandomGenerator());
	}

	//Set the objects in the factory
	aFactory->Set(std::move(sObjects));
	return true;
}

//----------------
// GetObjects  - JSourceObject2
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactoryT<JSourceObject2>* aFactory)
{
	if(aFactory->GetTag() != "") return false; //Only default tag here
	//auto ter = aEvent->GetSingle<TextEventRecord>();
	//jout << "texteventrecord " << ter->data << std::endl;

	// Generate a random # of objects
	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
	auto sNumObjects = sNumObjectsDistribution(gRandomGenerator);
	std::vector<JSourceObject2*> sObjects;
	double sSum = 0.0;
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		// Create new JSourceObject
		auto sObject = new JSourceObject2(gRandomGenerator(), si);
		sObjects.push_back( sObject );

#if 0
		// Supply busy work to take time. This would represent
		// the parsing of the input data.
		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(10, 200);
		auto sNumRandoms = sNumRandomsDistribution(gRandomGenerator);
		for(std::size_t sj = 0; sj < sNumRandoms; sj++) {

			double c = 0.0;
			for(int i=0; i<1000; i++){
				double a = gRandomGenerator();
				double b = sqrt( a*pow(1.23, -a))/a;
				c += b;
			}
			sObject->AddRandom(c);
			sSum += c;
		}
#endif
		sObject->SetHitE(sSum);
	}

	//Set the objects in the factory
	aFactory->Set(std::move(sObjects));
	return true;
}
