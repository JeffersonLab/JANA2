// $Id$
//
//    File: jana_test_factory.cc
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "jana_test_factory.h"
#include "JSourceObject.h"
#include "JSourceObject2.h"

#include <chrono>
#include <iostream>

//------------------
// Constructor
//------------------
jana_test_factory::jana_test_factory(void) : JFactoryT<jana_test>("jana_test_factory")
{
	//This is the new "init()" function

	//Seed random number generator //not ideal!
	auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	mRandomGenerator.seed(sTime);

}

//------------------
// Destructor
//------------------
jana_test_factory::~jana_test_factory(void)
{
	//This is the new "fini()" function
}


//------------------
// Init
//------------------
void jana_test_factory::Init(void) {}

//------------------
// ChangeRun
//------------------
void jana_test_factory::ChangeRun(const std::shared_ptr<const JEvent>& aEvent)
{
	if( GetPreviousRunNumber() != 0 )
		std::cout << "jana_test_factory::ChangeRun() called: run=" << aEvent->GetRunNumber() << "  (previous=" << GetPreviousRunNumber() << ")" << std::endl;
}

//------------------
// Process
//------------------
void jana_test_factory::Process(const std::shared_ptr<const JEvent>& aEvent)
{
	// This factory will grab the JSourceObject and JSourceObject2 types created by
	// the source. For each JSourceObject2 objects, it will create a jana_test
	// object (product of this factory). For each of those, it submits a JTask which
	// implements some busy work to represent calculations done to produce the jana_test
	// objects.

	// Get the JSourceObject and JSourceObject2 objects
	auto sobjs  = aEvent->Get<JSourceObject >();
	auto sobj2s = aEvent->Get<JSourceObject2>();

	// Create a jana_test object for each JSourceObject2 object
	std::vector< std::shared_ptr<JTaskBase> > sTasks;
	sTasks.reserve(sobj2s.size());
	for( auto sobj2 : sobj2s )
	{
		auto jtest = new jana_test();
		mData.push_back( jtest );

#if 1
		// Make a lambda that does busy work representing what would be done to
		// to calculate the jana_test objects from the inout objects.
		//
		// n.b. The JTask mechanism is hardwired to pass only the JEvent reference
		// as an argument. All other parameters must be passed as capture variables.
		// (e.g. the jtest and sObj2 objects)
		auto sMyLambda = [=](const std::shared_ptr<const JEvent> &aEvent) -> double
		{
			// Busy work
			std::mt19937 sRandomGenerator; // need dedicated generator to avoid thread conflict
			auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			sRandomGenerator.seed(sTime);
			double c = sobj2->GetHitE();
			for(int i=0; i<1000; i++){
				double a = sRandomGenerator();
				double b = sqrt( a*pow(1.23, -a))/a;
				c += b;
				jtest->AddRandom(a); // add to jana_test object
			}
			jtest->SetE( c ); // set energy of jana_test object

			return c/10.0; // more complicated than one normally needs but demonstrates use of a return value
		};

		// Make task shared_ptr (Return type of lambda is double)
		auto sTask = std::make_shared< JTask<double> >();

		// Set the event and the lambda
		sTask->SetEvent(aEvent);
		sTask->SetTask(std::packaged_task< double(const std::shared_ptr<const JEvent>&) >(sMyLambda));

		// Move the task onto the vector (to avoid changing shptr ref count). casts at the same time
		sTasks.push_back(std::move(sTask));
#endif
	}

#if 1
    // ------------ DISABLED--------------
    // This is related to the "DISABLED" note below so read that for more details.
    // The bottom line here is there are still problems with subtasks even after
    // commenting out the block at the bottom. Here, we don't use the subtasks
    // feature in JANA and instead just call each task directly.

	// Submit the tasks to the queues in the thread manager.
	// This function won't return until all of the tasks are finished.
	// This thread will help execute tasks (hopefully these) in the meantime.
	//(*aEvent).GetJApplication()->GetJThreadManager()->SubmitTasks(sTasks);
	for( auto sTask : sTasks ) (*sTask)();

	// ------------ DISABLED--------------
	// A change that went in on 3/25/2019 caused the get() method of the future
	// associated with each of the sTasks to be called right after the task
	// was run. This was to fix a problem that the future was holding on to
	// any exception thrown by the user's JEventProcessor causing it to be
	// silently ignored. A consequence of that is that the packaged_task is
	// left in an invalid state so when we call get() again (via JTask::GetResult)
	// an exception is thrown to indicate the future no longer has an associated
	// state.
	//
	// We disable this for now and will address the problem if it still exists
	// after the greenfield merge.
	//
	//  4/30/2019 DL
	//
	// For purposes of example, we grab the return values from the tasks
	//double sSum = 0.0;
	//for( auto sTask : sTasks ){
	//	auto &st = static_cast< JTask<double>& >(*sTask);
	//	sSum += st.GetResult();
	//}
#endif
}
