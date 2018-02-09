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
#include "JQueueInterface.h"
#include "JQueueSet.h"
#include "JLog.h"


thread_local JThread *JTHREAD = nullptr;

//---------------------------------
// JThread    (Constructor)
//---------------------------------
JThread::JThread(int aThreadID, JApplication* aApplication, JThreadManager::JEventSourceInfo* aSourceInfo, std::size_t aQueueSetIndex, bool aRotateEventSources) :
		mApplication(aApplication), mThreadManager(mApplication->GetJThreadManager()), mEventSourceInfo(aSourceInfo), mQueueSetIndex(aQueueSetIndex),
		mEventQueue(mEventSourceInfo->mQueueSet->GetQueue(JQueueSet::JQueueType::Events)), mRotateEventSources(aRotateEventSources),
		mThreadID(aThreadID)
{
	gPARMS->SetDefaultParameter("JANA:THREAD_DEBUG_LEVEL", mDebugLevel, "JThread(Manager) debug level");

	auto sSleepTimeNanoseconds = mSleepTime.count();
	gPARMS->SetDefaultParameter("JANA:THREAD_SLEEP_TIME_NS", sSleepTimeNanoseconds, "Thread sleep time (in nanoseconds) when nothing to do.");
	if(sSleepTimeNanoseconds != mSleepTime.count())
		mSleepTime = std::chrono::nanoseconds(sSleepTimeNanoseconds);

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
// GetThread
//---------------------------------
int JThread::GetThreadID(void) const
{
	return mThreadID;
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

	//Set logger
	mLogger = new JLog(0); //std::cout

	/// Loop continuously, processing events
	try{
		while( mRunStateTarget != kRUN_STATE_ENDED )
		{
			// If specified, go into idle state
			if( mRunStateTarget == kRUN_STATE_IDLE ) mRunState = kRUN_STATE_IDLE;

			// If not running, sleep and loop again
			if(mRunState != kRUN_STATE_RUNNING)
			{
				std::this_thread::sleep_for(mSleepTime); //Sleep a minimal amount.
				continue;
			}

			//Check if not enough event-tasks queued
			if(CheckEventQueue())
			{
				//Process-event task is submitted, redo the loop in case we want to buffer more
				continue;
			}

			// Grab the next task
			if(mDebugLevel >= 50)
				*mLogger << "Thread " << mThreadID << " JThread::Loop(): Grab task\n" << JLogEnd();
			auto sQueueType = JQueueSet::JQueueType::Events;
			auto sTask = std::shared_ptr<JTaskBase>(nullptr);
			std::tie(sQueueType, sTask) = mEventSourceInfo->mQueueSet->GetTask();
			if(mDebugLevel >= 50)
				*mLogger << "Thread " << mThreadID << " JThread::Loop(): Task pointer: " << sTask << "\n" << JLogEnd();

			//Do we have a task?
			if(sTask == nullptr)
			{
				if(!HandleNullTask())
					break; //No more tasks, end the loop
				continue;
			}

			//Execute task
			mThreadManager->ExecuteTask(sTask, mEventSourceInfo, sQueueType);

			//Task complete.  If this was an event task, rotate to next open file (if desired)
			//Don't rotate if it was a subtask or output task, because many of these may have submitted at once and we want to finish those first
			if(mRotateEventSources)
			{
				mFullRotationCheckIndex = mQueueSetIndex; //reset for full-rotation-with-no-action check
				if(sQueueType == JQueueSet::JQueueType::Events)
				{
					mEventSourceInfo = mThreadManager->GetNextSourceQueues(mQueueSetIndex);
					if(mEventSourceInfo == nullptr)
					{
						mRunStateTarget = kRUN_STATE_ENDED;
						continue; //Somehow down to no tasks left: We're done processing
					}
					if(mDebugLevel >= 20)
						*mLogger << "Thread " << mThreadID << " JThread::Loop(): Rotated sources to: " << mQueueSetIndex << ", rotation check = " << mFullRotationCheckIndex << "\n" << JLogEnd();
					mEventQueue = mEventSourceInfo->mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
					mSourceEmpty = false;
				}
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

	if(mDebugLevel >= 50)
		*mLogger << "Thread " << mThreadID << " JThread::CheckEventQueue(): Is-empty, enough buffered: " << mSourceEmpty << ", " << mEventQueue->AreEnoughTasksBuffered() << "\n" << JLogEnd();
	if(mSourceEmpty || mEventQueue->AreEnoughTasksBuffered())
		return false; //Didn't buffer more events, just execute the next available task

	//Figure out how many event (tasks) to get
	auto sNumEventsToGetRange = mEventSourceInfo->mEventSource->GetNumEventsToGetAtOnce();
	auto sNumEventsRangeSize = sNumEventsToGetRange.second - sNumEventsToGetRange.first;

	//The default is to get range max, unless the range size is non-zero or there are many threads
		//If only one thread: Get the max: maximize cache coherency, keep the disk hot
		//Or else we'll just have to keep getting from file one at a time over and over again
	auto sNumEventsToGet = sNumEventsToGetRange.second;
	if((sNumEventsRangeSize > 0) && (mThreadManager->GetNJThreads() > 1))
	{
		//The # of events we'll get is based on how full the buffer is.
		//If the buffer is nearly empty, we'll get very few events.
			//Why? So that way the other threads won't have to wait as long for something to do
		//If the buffer is nearly full, we'll get a lot of events
			//Why? Because the other threads have plenty of work to keep them busy.
			//We only want one thread worried about reading from the source.
			//Minimizes contention, keeps the disk hot, maximizes cache coherency
		//Note that the buffer shouldn't be thought of as a MAX, but as a MIN.
			//Thus we can skyrocket over it
		auto sBufferFillFraction = double(mEventQueue->GetNumTasks()) / double(mEventQueue->GetTaskBufferSize());
		if(sBufferFillFraction > 1.0)
			sBufferFillFraction = 1.0;
		sNumEventsToGet = sNumEventsToGetRange.first + sBufferFillFraction*(double(sNumEventsRangeSize));
	}

	if(mDebugLevel >= 20)
		*mLogger << "Thread " << mThreadID << " JThread::CheckEventQueue(): Get " << sNumEventsToGet << " process event tasks\n" << JLogEnd();

	//Get the next event(s) from the source, and get task(s) to process it/them (unless another thread has locked access)
	auto sTasksPair = mEventSourceInfo->mEventSource->GetProcessEventTasks(sNumEventsToGet);
	auto sReturnStatus = sTasksPair.second;
	auto& sEventTasks = sTasksPair.first;

	//Check if successful. If so, add tasks to queue
	if(sReturnStatus == JEventSource::RETURN_STATUS::kSUCCESS)
	{
		if(mDebugLevel >= 40)
			*mLogger << "Thread " << mThreadID << " JThread::CheckEventQueue(): Success, add task(s)\n" << JLogEnd();

		//Loop over tasks
		for(auto& sEventTask : sEventTasks)
		{
			//Add this process-event task to the event queue.
			//We have no where else to put this task if it doesn't fit in the queue.
			//So just continually try to add it
			if(mEventQueue->AddTask(std::move(sEventTask)) != JQueueInterface::Flags_t::kNO_ERROR)
			{
				//Add failed, queue must be full.
				//This (probably) shouldn't happen if the relationship between the
				//queue size and the buffer size is reasonable.

				//Oh well. Just execute this task directly instead
				//This is faster than pulling an event off the front of the queue
				(*sEventTask)();
			}
		}

		//Process-event task is submitted
		return true;
	}
	else if(sReturnStatus == JEventSource::RETURN_STATUS::kNO_MORE_EVENTS)
	{
		if(mDebugLevel >= 10)
			*mLogger << "Thread " << mThreadID << " JThread::CheckEventQueue(): No more events\n" << JLogEnd();

		//The source has been emptied.
		//Continue processing jobs from this source until there aren't any more
		mSourceEmpty = true;
		return false;
	}
	else if(sReturnStatus == JEventSource::RETURN_STATUS::kTRY_AGAIN)
	{
		if(mDebugLevel >= 10)
			*mLogger << "Thread " << mThreadID << " JThread::CheckEventQueue(): Try again later\n" << JLogEnd();
		return false;
	}

	//Else: Another thread has exclusive access to this event source
	//In this case, try to execute other jobs for this source (if none, will rotate-sources/sleep after check
	return false;
}

//---------------------------------
// HandleNullTask
//---------------------------------
bool JThread::HandleNullTask(void)
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

	//This returns false if there are no tasks left to process. Otherwise returns true.

	if(mDebugLevel >= 10)
		*mLogger << "Thread " << mThreadID << " JThread::HandleNullTask(): Null task: is empty, # outstanding = " << mSourceEmpty << ", " << mEventSourceInfo->mEventSource->GetNumOutstandingEvents() << "\n" << JLogEnd();
	if(mSourceEmpty && (mEventSourceInfo->mEventSource->GetNumOutstandingEvents() == 0))
	{
		//Tell the thread manager that the source is finished (in case it didn't know already)
		//Use the existing queues for the next event source
		//If all sources are done, then all tasks are done, and this call will tell the program to end.

		mEventSourceInfo = mThreadManager->RegisterSourceFinished(mEventSourceInfo->mEventSource, mQueueSetIndex);
		if(mEventSourceInfo == nullptr)
			return false; //No tasks left: We're done processing

		mEventQueue = mEventSourceInfo->mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
		mSourceEmpty = false;
		mFullRotationCheckIndex = mQueueSetIndex; //reset for full-rotation-with-no-action check
		return true;
	}

	if(mRotateEventSources) //No tasks, try to rotate to a different input file
	{
		mEventSourceInfo = mThreadManager->GetNextSourceQueues(mQueueSetIndex);
		if(mEventSourceInfo == nullptr)
			return false; //Somehow in between, down to no tasks left: We're done processing
		mEventQueue = mEventSourceInfo->mQueueSet->GetQueue(JQueueSet::JQueueType::Events);
		mSourceEmpty = false;
		if(mDebugLevel >= 20)
			*mLogger << "Thread " << mThreadID << " JThread::HandleNullTask(): Rotated sources to " << mQueueSetIndex << ", rotation check = " << mFullRotationCheckIndex << "\n" << JLogEnd();

		//Check if we have rotated through all open event sources without doing anything
		//If so, we are probably waiting for another thread to finish a task, or for more events to be ready.
		if(mQueueSetIndex == mFullRotationCheckIndex) //yep
			std::this_thread::sleep_for(mSleepTime); //Sleep a minimal amount.
	}
	else //No tasks, wait a bit for this event source to be ready
		std::this_thread::sleep_for(mSleepTime); //Sleep a minimal amount.

	return true;
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
