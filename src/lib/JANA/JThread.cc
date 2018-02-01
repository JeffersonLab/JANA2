//
//    File: JThread.cc
// Created: Wed Oct 11 22:51:22 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include <thread>
#include <chrono>
#include <iostream>
#include <tuple>

#include "JThread.h"
#include "JEvent.h"
#include "JThreadManager.h"
#include "JEventSource.h"
#include "JApplication.h"

thread_local JThread *JTHREAD = nullptr;

//---------------------------------
// JThread    (Constructor)
//---------------------------------
JThread::JThread(JApplication* aApplication, JQueueSet* aQueueSet, std::size_t aQueueSetIndex, JEventSource* aSource, bool aRotateEventSources) :
		mApplication(aApplication), mThreadManager(mApplication->GetJThreadManager()), mQueueSet(aQueueSet), mQueueSetIndex(aQueueSetIndex),
		mEventSource(aSource), mEventQueue(mQueueSet->GetQueue(JQueueSet::JQueueType::Events)), mRotateEventSources(aRotateEventSources)
{
	_thread = new std::thread( &JThread::Loop, this );
}

//---------------------------------
// ~JThread    (Destructor)
//---------------------------------
JThread::~JThread()
{
//extern void WriteBuff(void);
//WriteBuff();
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
uint64_t JThread::GetNumEventsProcessed(void)
{
	/// Get total number of events processed by this thread. This
	/// returns a total of all "events" from all queues. Since the
	/// nature of events from different queues differs (e.g. one
	/// event in queue "A" may turn into 100 events in queue "B")
	/// this value should be used with caution. See the other form
	/// GetNeventsProcessed(map<string,uint64_t> &) which returns
	/// counts for individual queues for a perhaps more useful breakdown.
	
	std::map<std::string,uint64_t> Nevents;
	GetNumEventsProcessed(Nevents);
	
	uint64_t Ntot = 0;
	for(auto p : Nevents) Ntot += p.second;
	
	return Ntot;
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
void JThread::GetNumEventsProcessed(std::map<std::string,uint64_t> &Nevents)
{
	/// Get number of events processed by this thread for each
	/// JQueue. The key will be the name of the JQueue and value
	/// the number of events from that queue processed. The returned
	/// map is not guaranteed to have an entry for each JQueue since
	/// it is possible this thread has not processed events from all
	/// JQueues.
	
	Nevents = _events_processed;
}

//---------------------------------
// GetThread
//---------------------------------
std::thread* JThread::GetThread(void)
{
	/// Get the C++11 thread object.

	return _thread;
}

//---------------------------------
// Join
//---------------------------------
void JThread::Join(void)
{
	/// Join this thread. If the thread is not already in the ended
	/// state then this will call End() and wait for it to do so
	/// before calling join. This should generally only be called 
	/// from a method JThreadManager.
	if(_isjoined)
		return;
	if( mRunStateTarget != kRUN_STATE_ENDED ) End();
	while( mRunState != kRUN_STATE_ENDED ) std::this_thread::sleep_for( std::chrono::microseconds(100) );
	_thread->join();
	_isjoined = true;
}

//---------------------------------
// End
//---------------------------------
void JThread::End(void)
{
	/// Stop the thread from processing events and end operation
	/// completely. If an event is currently being processed by the
	/// thread then that will complete first. The JThread will then
	/// exit. If you wish to stop processing of events temporarily
	/// without exiting the thread then use Stop().
	mRunStateTarget = kRUN_STATE_ENDED;
	
	// n.b. to implement a "wait_until_ended" here we would need
	// to do somethimg like register a flag that gets set just
	// before the thread exits since once it does, we can't actually
	// access the JThread (i.e. the "this" pointer is will cease
	// to be valid). Implementation of that is deferred until the
	// need is made clear.
}

//---------------------------------
// IsIdle
//---------------------------------
bool JThread::IsIdle(void)
{
	/// Return true if the thread is currently in the idle state.
	/// Being in the idle state means the thread is waiting to be
	/// told to start processing events (this does not mean it is
	/// waiting for an event to show up in the queue). Use this
	/// after calling Stop() to know when the thread has finished
	/// processing its current event.
	
	return mRunState == kRUN_STATE_IDLE;
}

//---------------------------------
// IsEnded
//---------------------------------
bool JThread::IsEnded(void)
{
	return mRunState == kRUN_STATE_ENDED;
}

//---------------------------------
// IsJoined
//---------------------------------
bool JThread::IsJoined(void)
{
	return _isjoined;
}

//---------------------------------
// Loop
//---------------------------------
void JThread::Loop(void)
{
	// Set thread_local global variable
	JTHREAD = this;

	/// Loop continuously, processing events
	try{
		while( mRunStateTarget != kRUN_STATE_ENDED )
		{
			// If specified, go into idle state
			if( mRunStateTarget == kRUN_STATE_IDLE ) mRunState = kRUN_STATE_IDLE;

			// If not running, sleep and loop again
			if(mRunState != kRUN_STATE_RUNNING)
			{
				std::this_thread::sleep_for( std::chrono::nanoseconds(100) ); //Sleep a minimal amount.
				continue;
			}

			//Check if not enough event-tasks queued
			if(CheckEventQueue())
				continue; //we buffered more event-tasks, redo the loop

			// Grab the next task
//			std::cout << "Grab task\n";
			auto sQueueType = JQueueSet::JQueueType::Events;
			auto sTask = std::shared_ptr<JTaskBase>(nullptr);
			std::tie(sQueueType, sTask) = mQueueSet->GetTask();

			std::cout << "Task pointer: " << sTask << "\n";

			//Do we have a task?
			if(sTask == nullptr)
			{
				HandleNullTask();
				continue;
			}

			//If from the event queue, add factories to the event (JFactorySet)
			//We don't add them eariler (e.g. in the event source) because it would take too much memory
			//E.g. if 200 events in the queue, we'd have 200 factories of each type
			//Instead, only the events that are actively being analyzed have factories
			//Once the JEvent goes out of scope and releases the JFactorySet, it returns to the pool
			if(sQueueType == JQueueSet::JQueueType::Events)
			{
				//OK, this is a bit ugly
				//The problem is that JTask holds a shared_ptr of const JEvent*, and we need to modify it.

				//Why not just hold a non-const shared_ptr?
				//Because we don't want the user calling things like JEvent::SetEventNumber() in their factories.

				//So what do we do? The dreaded const_cast.
				auto sEvent = const_cast<JEvent*>(sTask->GetEvent());

				//Is there something else we can do instead?
				//We could pass the JFactorySet alongside the JEvent instead of as a member of it.
				//But the user is not intended to ever directly interact with the JFactorySet
				//And we'll also have to deal with this again for sending along barrier-event data.
				//This ends up requiring a lot of arguments to factory/processor/task methods.

				//Finally, by having the JEvent Release() function recycle the JFactorySet, it doesn't have to be shared_ptr
				//If it's shared_ptr: more (slow) atomic operations
				//Just make the JFactorySet a member and cheat a little bit. It's a controlled environment.
				sEvent->SetFactorySet(mApplication->GetFactorySet());
			}

			//Execute task
			(*sTask)();

			//Task complete.  If this was an event task, rotate to next open file (if desired)
			//Don't rotate if it was a subtask or output task, because many of these may have submitted at once and we want to finish those first
//			std::cout << "rotate flag, event-check: " << mRotateEventSources << ", " << (sQueueType == JQueueSet::JQueueType::Events) << "\n";
			if(mRotateEventSources && (sQueueType == JQueueSet::JQueueType::Events))
			{
				std::cout << "rotate sources\n";
				mFullRotationCheckIndex = mQueueSetIndex; //reset for full-rotation-with-no-action check
				std::tie(mEventSource, mQueueSet) = mThreadManager->GetNextSourceQueues(mQueueSetIndex);
				mEventQueue = mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
				mSourceEmpty = false;
			}
		}
	}catch(JException &e){
		jerr << "** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** " << std::endl;
		jerr << "Caught JException: " << e.GetMessage() << std::endl;
		jerr << "** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** " << std::endl;
	}

	// Set flag that we're done just before exiting thread
	mRunState = kRUN_STATE_ENDED;
}

//---------------------------------
// CheckEventQueue
//---------------------------------
bool JThread::CheckEventQueue(void)
{
	//Returns true if the event-task buffer is too low and we read-in/prepared new events
	//Otherwise (buffer is high enough or there are no events) return false

//	std::cout << "is-empty, enough buffered: " << mSourceEmpty << ", " << mEventQueue->AreEnoughTasksBuffered() << "\n";
	if(mSourceEmpty || mEventQueue->AreEnoughTasksBuffered())
		return false; //Didn't buffer more events, just execute the next available task

	std::cout << "Get process event task\n";

	//Not enough queued. Get the next event from the source, and get a task to process it (unless another thread has locked access)
	auto sReturnStatus = JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
	auto sEventTask = std::shared_ptr<JTaskBase>(nullptr);
	std::tie(sEventTask, sReturnStatus) = mEventSource->GetProcessEventTask();

	if(sReturnStatus == JEventSource::RETURN_STATUS::kSUCCESS)
	{
//			std::cout << "Success, add task\n";
		//Add the process-event task to the event queue.
		mEventQueue->AddTask(std::move(sEventTask));
		return true;
	}
	else if(sReturnStatus == JEventSource::RETURN_STATUS::kNO_MORE_EVENTS)
	{
		std::cout << "No more events.\n";
		//The source has been emptied.
		//Continue processing jobs from this source until there aren't any more
		mSourceEmpty = true;
		return false;
	}
	else if(sReturnStatus == JEventSource::RETURN_STATUS::kTRY_AGAIN)
	{
		std::cout << "Try again later.\n";
		return false;
	}

	//Else: Another thread has exclusive access to this event source
	//In this case, try to execute other jobs for this source (if none, will rotate-sources/sleep after check
	return false;
}

//---------------------------------
// HandleNullTask
//---------------------------------
void JThread::HandleNullTask(void)
{
	//There are no tasks in the queue for this event source.  Are we done with the source?
	//We are not done with a source until all tasks referencing it have completed.

	//Just because there isn't a task in any of the queues doesn't mean we are done:
	//A thread may have removed a task and be currently running it.
	//Also, that thread may spawn a bunch of sub-tasks, so we still want all threads
	//to be available to process them.

	//So, how can we tell if all threads are done processing tasks from a given file?
	//When all JEvent's associated with that file are destroyed/recycled.
	//We track this by counting the number of open JEvent's in each JEventSource.

	std::cout << "null task: is empty, # outstanding = " << mSourceEmpty << ", " << mEventSource->GetNumOutstandingEvents() << "\n";
	if(mSourceEmpty && (mEventSource->GetNumOutstandingEvents() == 0))
	{
		//Tell the thread manager that the source is finished (in case it didn't know already)
		//Use the existing queues for the next event source
		//If all sources are done, then all tasks are done, and this call will tell the program to end.

		std::tie(mEventSource, mQueueSet) = mThreadManager->RegisterSourceFinished(mEventSource, mQueueSetIndex);
		if(mQueueSet != nullptr)
			mEventQueue = mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
		mSourceEmpty = false;
		mFullRotationCheckIndex = mQueueSetIndex; //reset for full-rotation-with-no-action check
		return;
	}

	if(mRotateEventSources) //No tasks, try to rotate to a different input file
	{
		std::cout << "rotate sources\n";
		std::tie(mEventSource, mQueueSet) = mThreadManager->GetNextSourceQueues(mQueueSetIndex);
		mEventQueue = mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
		mSourceEmpty = false;

		//Check if we have rotated through all open event sources without doing anything
		//If so, we are probably waiting for another thread to finish a task, or for more events to be ready.
		if(mQueueSetIndex == mFullRotationCheckIndex) //yep
			std::this_thread::sleep_for( std::chrono::nanoseconds(100) ); //Sleep a minimal amount.
	}
	else //No tasks, wait a bit for this event source to be ready
		std::this_thread::sleep_for( std::chrono::nanoseconds(100) ); //Sleep a minimal amount.
}

//---------------------------------
// Run
//---------------------------------
void JThread::Run(void)
{
	/// Start this thread processing events. This simply
	/// sets the run state flag. 

	mRunState = mRunStateTarget = kRUN_STATE_RUNNING;
}

//---------------------------------
// Stop
//---------------------------------
void JThread::Stop(bool wait_until_idle)
{
	/// Stop the thread from processing events. The stop will occur
	/// between events so if an event is currently being processed,
	/// that will complete before the thread goes idle. If wait_until_idle
	/// is true, then this will not return until the thread is no longer
	/// in the "running" state and therefore no longer processing events.
	/// This will usually be because the thread has gone into the idle
	/// state, but may also be because the thread has been told to end
	/// completely.
	/// Use IsIdle() to check if the thread is in the idle
	/// state.
	mRunStateTarget = kRUN_STATE_IDLE;
	
	// The use of yield() here will (according to stack overflow) cause
	// the core to be 100% busy while waiting for the state to change.
	// That should only be for the duration of processing of the current
	// event though and this method is expected to be used rarely so
	// this should be OK.
	if( wait_until_idle ) while( mRunState == kRUN_STATE_RUNNING ) std::this_thread::yield();
}
