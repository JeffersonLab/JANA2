//
//    File: JQueueWithBarriers.cc
// Created: Wed Oct 11 22:51:32 EDT 2017
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

#include <iostream>
#include "JQueueWithBarriers.h"
#include "JLog.h"
#include "JThread.h"
#include "JEvent.h"

//---------------------------------
// JQueueWithBarriers    (Constructor)
//---------------------------------
JQueueWithBarriers::JQueueWithBarriers(const std::string& aName, std::size_t aQueueSize, std::size_t aTaskBufferSize) :
	JQueueInterface(aName), mTaskBufferSize(aTaskBufferSize), mInputQueue(new JQueue(aName + " Input", aQueueSize, aTaskBufferSize)),
	mOutputQueue(new JQueue(aName + " Output", aQueueSize, aTaskBufferSize))
{
	gPARMS->SetDefaultParameter("JANA:QUEUE_DEBUG_LEVEL", mDebugLevel, "JQueue debug level");
	mThread = new std::thread(&JQueueWithBarriers::ThreadLoop, this);
}

//---------------------------------
// JQueueWithBarriers    (Copy Constructor)
//---------------------------------
JQueueWithBarriers::JQueueWithBarriers(const JQueueWithBarriers& aQueue) : JQueueInterface(aQueue)
{
	//Assume this is called by CloneEmpty() or similar on an empty queue (ugh, can improve later)
	mTaskBufferSize = aQueue.mTaskBufferSize;
	mDebugLevel = aQueue.mDebugLevel;
	mLogTarget = aQueue.mLogTarget;
	mInputQueue = static_cast<JQueue*>(aQueue.mInputQueue->CloneEmpty());
	mOutputQueue = static_cast<JQueue*>(aQueue.mOutputQueue->CloneEmpty());

	mThread = new std::thread(&JQueueWithBarriers::ThreadLoop, this);
}

//---------------------------------
// operator=
//---------------------------------
JQueueWithBarriers& JQueueWithBarriers::operator=(const JQueueWithBarriers& aQueue)
{
	//Assume this is called by Clone() or similar on an empty queue (ugh, can improve later)
	JQueueInterface::operator=(aQueue);
	mTaskBufferSize = aQueue.mTaskBufferSize;
	return *this;
}

//---------------------------------
// ~JQueueWithBarriers    (Destructor)
//---------------------------------
JQueueWithBarriers::~JQueueWithBarriers(void)
{
	mEndThread = true;
	mThread->join();
	delete mThread;
}

//---------------------------------
// AddTask
//---------------------------------
JQueueInterface::Flags_t JQueueWithBarriers::AddTask(const std::shared_ptr<JTaskBase>& aTask)
{
	return mInputQueue->AddTask(aTask);
}

//---------------------------------
// AddTask
//---------------------------------
JQueueInterface::Flags_t JQueueWithBarriers::AddTask(std::shared_ptr<JTaskBase>&& aTask)
{
	return mInputQueue->AddTask(std::move(aTask));
}

//---------------------------------
// GetMaxTasks
//---------------------------------
uint32_t JQueueWithBarriers::GetMaxTasks(void)
{
	/// Returns maximum number of Tasks queue can hold at one time.
	return mInputQueue->GetMaxTasks() + mOutputQueue->GetMaxTasks();
}

//---------------------------------
// JQueueWithBarriers
//---------------------------------
void JQueueWithBarriers::ThreadLoop(void)
{
	//This thread is responsible for moving tasks from the input queue to the output queue.
	//It will do so until it encounters a barrier event.
	//At that point, no further tasks are moved into the output queue until the task with the barrier event is done.

	//This way, future events from this event source have the complete set of data from the barrier event.
	//Otherwise, another thread could analyze the next event and not have the complete set of information.
	//Note that every task in this queue is from the same event source.

	//Unforunately, we can't use a single JQueue to do this.
	//This is because, when getting tasks, once we get exclusive access to a slot, another thread
		//is able to get the task from the following slot.
		//We would need to check whether we had a barrier event and prevent the subsequent GetTask in a single operation
		//which is impossible. Otherwise, we'd have to lock a mutex in JQueue.
	while(!mEndThread)
	{
		//Move all tasks from the input queue to the output queue, until we hit a barrier
		while(!mEndThread)
		{
			//Get task from input queue
			auto sTask = mInputQueue->GetTask();
			if(sTask == nullptr)
				break; //No more tasks to move

			//Is it a barrier event?
			auto sIsBarrierEvent = sTask->GetEvent()->GetIsBarrierEvent();
			if(sIsBarrierEvent)
			{
				//Hold a weak pointer to the task that is responsible for running the plugins over this event
				mAnalyzeBarrierEventTask = sTask;
			}

			//Move task to output queue
			mOutputQueue->AddTask(std::move(sTask));

			//Don't move any more tasks to the output queue until the barrier event is finished being analyzed.
			if(sIsBarrierEvent)
			{
				//When is it done being analyzed? When the weak_ptr has expired
				while(!mAnalyzeBarrierEventTask.expired() && !mEndThread)
					std::this_thread::sleep_for(mSleepTimeIfBarrier); //Wait
			}
		}

		//All tasks moved, sleep a while to wait for more
		std::this_thread::sleep_for(mSleepTime);
	}
}

//---------------------------------
// GetEvent
//---------------------------------
std::shared_ptr<JTaskBase> JQueueWithBarriers::GetTask(void)
{
	return mOutputQueue->GetTask();
}

//---------------------------------
// GetNumTasks
//---------------------------------
uint32_t JQueueWithBarriers::GetNumTasks(void)
{
	/// Returns the number of tasks currently in this queue.
	return mInputQueue->GetNumTasks() + mOutputQueue->GetNumTasks();
}

//---------------------------------
// GetNumTasksProcessed
//---------------------------------
uint64_t JQueueWithBarriers::GetNumTasksProcessed(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumTasks for that).
	return mOutputQueue->GetNumTasksProcessed();
}

//---------------------------------
// AreEnoughTasksBuffered
//---------------------------------
bool JQueueWithBarriers::AreEnoughTasksBuffered(void)
{
	//This function is only called for the Event queue
	//If not enough tasks (events) are buffered, we will read in more events
	return mInputQueue->AreEnoughTasksBuffered();
}

//---------------------------------
// GetTaskBufferSize
//---------------------------------
std::size_t JQueueWithBarriers::GetTaskBufferSize(void)
{
	return mInputQueue->GetTaskBufferSize();
}

//---------------------------------
// Clone
//---------------------------------
JQueueInterface* JQueueWithBarriers::CloneEmpty(void) const
{
	//Create an empty clone of the queue (no tasks copied)
	return (new JQueueWithBarriers(*this));
}
