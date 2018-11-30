//
//    File: JQueue.cc
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
#include "JQueue.h"
#include "JLog.h"
#include "JThread.h"

//---------------------------------
// JQueue    (Constructor)
//---------------------------------
JQueue::JQueue(const std::string& aName, std::size_t aQueueSize, std::size_t aTaskBufferSize) : JQueueInterface(aName), mTaskBufferSize(aTaskBufferSize)
{
	//Apparently segfaults
//	gPARMS->SetDefaultParameter("JANA:QUEUE_DEBUG_LEVEL", mDebugLevel, "JQueue debug level");
	mNslots = aQueueSize;
	mQueue.resize(mNslots);
	
	mWriteSlotptr = new std::atomic<uint32_t>[mNslots];
	mReadSlotptr  = new std::atomic<uint32_t>[mNslots];
}

//---------------------------------
// ~JQueue    (Destructor)
//---------------------------------
JQueue::~JQueue()
{
	if( mWriteSlotptr != nullptr ) delete[] mWriteSlotptr;
	if( mReadSlotptr  != nullptr ) delete[] mReadSlotptr;
}

//---------------------------------
// JQueue    (Copy Constructor)
//---------------------------------
JQueue::JQueue(const JQueue& aQueue) : JQueueInterface(aQueue)
{
	//Assume this is called by CloneEmpty() or similar on an empty queue (ugh, can improve later)
	mTaskBufferSize = aQueue.mTaskBufferSize;
	mDebugLevel = aQueue.mDebugLevel;
	mLogTarget = aQueue.mLogTarget;
	mNslots = aQueue.mNslots;
	mQueue.resize(mNslots);
	
	if( mWriteSlotptr != nullptr ) delete[] mWriteSlotptr;
	if( mReadSlotptr  != nullptr ) delete[] mReadSlotptr;
	mWriteSlotptr = new std::atomic<uint32_t>[mNslots];
	mReadSlotptr  = new std::atomic<uint32_t>[mNslots];
}

//---------------------------------
// operator=
//---------------------------------
JQueue& JQueue::operator=(const JQueue& aQueue)
{
	//Assume this is called by Clone() or similar on an empty queue (ugh, can improve later)
	JQueueInterface::operator=(aQueue);
	mTaskBufferSize = aQueue.mTaskBufferSize;
	mDebugLevel = aQueue.mDebugLevel;
	mLogTarget = aQueue.mLogTarget;
	mNslots = aQueue.mNslots;
	mQueue.resize(mNslots);
	
	if( mWriteSlotptr != nullptr ) delete[] mWriteSlotptr;
	if( mReadSlotptr  != nullptr ) delete[] mReadSlotptr;
	mWriteSlotptr = new std::atomic<uint32_t>[mNslots];
	mReadSlotptr  = new std::atomic<uint32_t>[mNslots];

	return *this;
}

//---------------------------------
// AddTask
//---------------------------------
JQueueInterface::Flags_t JQueue::AddTask(const std::shared_ptr<JTaskBase>& aTask)
{
	//We want to copy the task into the queue instead of moving it.
	//However, we don't want to duplicate the complicated code in both functions.
	//Instead, we will create a local copy, and then call the other method with it.

	//We need to create a local copy first because we will move from it
	//And we don't want to move from the input (the whole point is to make a COPY).

	//This will incur a ref-count change due to copy, but we had to do that anyway.
	//The additional move into the queue is cheap (2 pointers),
	//and is worth avoiding the code duplication (or confusion of other tricks)

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JQueue::AddTask(): Copy-add task " << aTask.get() << "\n" << JLogEnd();

	auto sTempTask = aTask;
	return AddTask(std::move(sTempTask));
}

//---------------------------------
// AddTask
//---------------------------------
JQueueInterface::Flags_t JQueue::AddTask(std::shared_ptr<JTaskBase>&& aTask)
{
	// Loop over write slots to get exclusive write access to one
	auto sptr = mWriteSlotptr;
	for(uint32_t idx=0; idx<mNslots; idx++, sptr++){
		uint32_t in_use = false;
		if( sptr->compare_exchange_weak(in_use, true) ){

			// We now have exclusive access to slot.
			// Move task into slot and flag it as ready
			// to be read.
			mQueue[idx] = std::move(aTask);
			mReadSlotptr[idx] = true;
			mTasksInserted++;

			if(mDebugLevel > 0) JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JQueue::AddTask(): Task added to slot " << idx << ".\n" << JLogEnd();
			
			return Flags_t::kNO_ERROR;
		}
	}

	// No slots were available. Let caller know
	return Flags_t::kQUEUE_FULL;
}

//---------------------------------
// AddTasksProcessedOutsideQueue
//---------------------------------
void JQueue::AddTasksProcessedOutsideQueue(std::size_t nTasks)
{
	mTasksRunOutsideQueue.fetch_add( nTasks );
}

//---------------------------------
// GetMaxTasks
//---------------------------------
uint32_t JQueue::GetMaxTasks(void)
{
	/// Returns maximum number of Tasks queue can hold at one time.
	return mQueue.size();
}

//---------------------------------
// GetTask
//---------------------------------
std::shared_ptr<JTaskBase> JQueue::GetTask(void)
{
	// Loop over read slots to get exclusive write access to one
	auto sptr = mReadSlotptr;
	for(uint32_t idx=0; idx<mNslots; idx++, sptr++){
		uint32_t has_data = true;
		if( sptr->compare_exchange_weak(has_data, false) ){

			// We now have exclusive access to slot.
			// Move task out of slot and flag it as available
			auto sTask = std::move(mQueue[idx]);
			mWriteSlotptr[idx] = false;
 			mTasksProcessed++;

			if(mDebugLevel > 0) JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JQueue::GetTask(): Task removed from slot " << idx << ".\n" << JLogEnd();
			
			return sTask; //should be copy-elided
		}
	}

	// No slots had tasks. Let caller know
	return nullptr;
}


//---------------------------------
// GetNumTasks
//---------------------------------
uint32_t JQueue::GetNumTasks(void)
{
	/// Returns the number of tasks currently in this queue.
	
	uint32_t nTasks = 0;
	auto sptr = mReadSlotptr;
	for(std::size_t islot=0; islot<mNslots; islot++, sptr++) if( *sptr ) nTasks++;
	
	return nTasks;
}

//---------------------------------
// GetNumTasksInserted
//---------------------------------
uint64_t JQueue::GetNumTasksInserted(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumTasks for that).

	return mTasksInserted;
}

//---------------------------------
// GetNumTasksProcessed
//---------------------------------
uint64_t JQueue::GetNumTasksProcessed(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumTasks for that).
	
	return mTasksProcessed + mTasksRunOutsideQueue;
}

//---------------------------------
// GetTaskBufferSize
//---------------------------------
std::size_t JQueue::GetTaskBufferSize(void)
{
	return mTaskBufferSize;
}

//---------------------------------
// AreEnoughTasksBuffered
//---------------------------------
bool JQueue::AreEnoughTasksBuffered(void)
{
	//This function is only called for the Event queue
	//If not enough tasks (events) are buffered, we will read in more events
//	std::cout << "num tasks, buffer size = " << GetNumTasks() << ", " << mTaskBufferSize << "\n";
	return (mTaskBufferSize == 0) ? (GetNumTasks() >= 1) : (GetNumTasks() >= mTaskBufferSize);
}

//---------------------------------
// Clone
//---------------------------------
JQueueInterface* JQueue::CloneEmpty(void) const
{
	//Create an empty clone of the queue (no tasks copied)
	return (new JQueue(*this));
}
