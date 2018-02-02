// $Id$
//
//    File: JEventSource_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "JEventSource_jana_test.h"
#include "JEvent_test.h"
#include "JSourceFactoryGenerator.h"
#include "JTask.h"

//----------------
// Constructor
//----------------
JEventSource_jana_test::JEventSource_jana_test(const char* source_name) : JEventSource(source_name, japp)
{
	mNumEventsToGenerate = 20000;

	//Seed random number generator //not ideal!
	auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	mRandomGenerator.seed(sTime);

	//Make factory generator that will make factories for all types provided by the event source
	//Make sure that all types are listed as template arguments here!!
	mFactoryGenerator = new JSourceFactoryGenerator<JSourceObject, JSourceObject2>();

	//Event queue:
	//If not created, a default will be supplied
	mEventQueue = new JQueue("Events", 200, 50); //max size of 200, keep at least 50 buffered
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

	//For all of the objects retrievable from the file, we MUST generate factories via our own JFactoryGenerator
	auto sTypeIndex = aFactory->GetObjectType();
	if(sTypeIndex == std::type_index(typeid(JSourceObject)))
		return GetObjects(aEvent, static_cast<JFactory<JSourceObject>*>(aFactory));
	else if(sTypeIndex == std::type_index(typeid(JSourceObject2)))
		return GetObjects(aEvent, static_cast<JFactory<JSourceObject2>*>(aFactory));
	else
		return false;
}

//----------------
// GetObjects
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory<JSourceObject>* aFactory)
{
	//You may need to lock on your event source here, depending on
	if(aFactory->GetTag() != "")
		return false; //Only default tag here

	//Generate a random # of objects
	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
	auto sNumObjects = sNumObjectsDistribution(mRandomGenerator);

	//Prepare the vector
	std::vector<JSourceObject> sObjects;
	sObjects.reserve(sNumObjects);

	//Make the objects
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		//Emplace new object onto the back of the data vector
		sObjects.emplace_back(mRandomGenerator(), si); //Random energy, id = object#
		auto& sObject = sObjects.back(); //Get reference to new object

		//Supply busy work to take time: Generate a bunch of randoms
		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(1000, 2000);
		auto sNumRandoms = sNumRandomsDistribution(mRandomGenerator);
		for(std::size_t sj = 0; sj < sNumRandoms; sj++)
			sObject.AddRandom(mRandomGenerator());
	}

	//Set the objects in the factory
	aFactory->Set(std::move(sObjects));
	return true;
}

//----------------
// GetObjects
//----------------
bool JEventSource_jana_test::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory<JSourceObject2>* aFactory)
{
	//You may need to lock on your event source here, depending on
	if(aFactory->GetTag() != "")
		return false; //Only default tag here

	//Generate a random # of objects
	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 100);
	auto sNumObjects = sNumObjectsDistribution(mRandomGenerator);

	//Make lambda (and std::packaged_task wrapping it) for generating random #'s
	//We will submit these in JTask's for multithreading work
	std::size_t sMinNumRandoms = 1000, sMaxNumRandoms = 2000;
	//We must pass the JEvent as an argument, even if we don't use it.
	//TODO: Consider improving JTask class to allow no JEvent (specialize)
	auto sGenerateRandoms = [sMinNumRandoms, sMaxNumRandoms](const std::shared_ptr<const JEvent>&) -> std::vector<double>
	{
		//Define our own random # generator
		//Don't use the one in the JEventSource: Not thread safe
		std::mt19937 sRandomGenerator;
		auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		sRandomGenerator.seed(sTime);

		//Supply busy work to take time: Generate a bunch of randoms
		//First generate how many randoms to generate (random # between min & max)
		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(sMinNumRandoms, sMaxNumRandoms);
		auto sNumRandoms = sNumRandomsDistribution(sRandomGenerator);

		//Prepare result vector
		std::vector<double> sRandoms;
		sRandoms.reserve(sNumRandoms);

		//Generate random #'s
		for(std::size_t sj = 0; sj < sNumRandoms; sj++)
			sRandoms.push_back(sRandomGenerator());

		//Compiler can auto-move, and even use return value optimization (RVO) to optimize-away the move!
		return sRandoms;
	};
	auto sPackagedTask = std::packaged_task<std::vector<double>(const std::shared_ptr<const JEvent>&)>(sGenerateRandoms);

	//Create tasks to create the random #'s for the objects
	std::vector<std::shared_ptr<JTaskBase>> sTasks;
	sTasks.reserve(sNumObjects);
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		//Make task shared_ptr (Return type of lambda is vector<double>)
		auto sTask = std::make_shared<JTask<std::vector<double>>>();

		//Set the event and the lambda
		sTask->SetEvent(aEvent);
		sTask->SetTask(std::move(sPackagedTask));

		//Move the task onto the vector (to avoid changing ref count)
		sTasks.push_back(std::move(sTask));
	}

	//Submit the tasks to the queues in the thread manager.
	//This function won't return until all of the tasks are finished.
	//This thread will execute tasks (hopefully these) in the meantime.
	mApplication->GetJThreadManager()->SubmitTasks(sTasks);

	//Prepare the object vector
	std::vector<JSourceObject2> sObjects;
	sObjects.reserve(sNumObjects);

	//Make the objects
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		//Emplace new object onto the back of the data vector
		sObjects.emplace_back(mRandomGenerator(), si); //Random energy, id = object#
		auto& sObject = sObjects.back(); //Get reference to new object

		//Move the randoms into the object
		//First get the raw JTaskBase* and cast it to the original, derived type
		auto sTask = static_cast<JTask<std::vector<double>>*>(sTasks[si].get());
		sObject.MoveRandoms(std::move(sTask->GetResult()));
	}

	//Set the objects in the factory
	aFactory->Set(std::move(sObjects));
	return true;
}
