//
//    File: JQueueWithLock.cc
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
#include "JQueueWithLock.h"
#include "JLogger.h"
#include "JThread.h"

//---------------------------------
// JQueueWithLock    (Constructor)
//---------------------------------
JQueueWithLock::JQueueWithLock(JParameterManager* aParams, 
		               const std::string& aName, 
			       std::size_t aQueueSize, 
			       std::size_t aTaskBufferSize) 

  : JQueue(aName), mQueueSize(aQueueSize), mTaskBufferSize(aTaskBufferSize) {

	aParams->SetDefaultParameter("JANA:QUEUE_DEBUG_LEVEL", 
				     mDebugLevel, 
				     "JQueueWithLock debug level");
}

//---------------------------------
// JQueueWithLock    (Copy Constructor)
//---------------------------------
JQueueWithLock::JQueueWithLock(const JQueueWithLock& aQueue) : JQueue(aQueue)
{
	//Assume this is called by CloneEmpty() or similar on an empty queue (ugh, can improve later)
	mTaskBufferSize = aQueue.mTaskBufferSize;
	mQueueSize = aQueue.mQueueSize;
	mDebugLevel = aQueue.mDebugLevel;
	mLogTarget = aQueue.mLogTarget;
}

//---------------------------------
// operator=
//---------------------------------
JQueueWithLock& JQueueWithLock::operator=(const JQueueWithLock& aQueue)
{
	//Assume this is called by Clone() or similar on an empty queue (ugh, can improve later)
	JQueue::operator=(aQueue);
	mTaskBufferSize = aQueue.mTaskBufferSize;
	mQueueSize = aQueue.mQueueSize;
	return *this;
}

//---------------------------------
// AddTask
//---------------------------------
JQueue::Flags_t JQueueWithLock::AddTask(const std::shared_ptr<JTaskBase>& aTask)
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
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JQueueWithLock::AddTask(): Copy-add task " << aTask.get() << "\n" << JLogEnd();

	auto sTempTask = aTask;
	return AddTask(std::move(sTempTask));
}

//---------------------------------
// AddTask
//---------------------------------
JQueue::Flags_t JQueueWithLock::AddTask(std::shared_ptr<JTaskBase>&& aTask)
{
	/// Add the given JTaskBase to this queue. This will do so without locks.
	/// If the queue is full, it will return immediately with a value
	/// of JQueueWithLock::kQUEUE_FULL. Upon success, it will return JQueueWithLock::NO_ERROR.

	// The queue is maintained by 4 atomic indices. The goal of this
	// method is to increment both the iwrite and iend indices so
	// they point to the same slot upon exit. The JTaskBase pointer will
	// be copied into the slot just in front of the one these point
	// to, making it available to the GetEvent method. If it sees that
	// the queue is full, it returns immediately. (This is to give the
	// calling thread a chance to do something else or call this again.
	// for another try.)

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JQueueWithLock::AddTask(): Move-add task " << aTask.get() << ".\n" << JLogEnd();

	std::lock_guard<std::mutex> sLock(mQueueLock);

	if( mQueue.size() >= mQueueSize ) return Flags_t::kQUEUE_FULL;  // Queue is already full

	mQueue.push_back(std::move(aTask));

	return Flags_t::kNO_ERROR;
}

//---------------------------------
// GetMaxTasks
//---------------------------------
uint32_t JQueueWithLock::GetMaxTasks(void)
{
	/// Returns maximum number of Tasks queue can hold at one time.
	return mQueueSize;
}

//---------------------------------
// GetTask
//---------------------------------
std::shared_ptr<JTaskBase> JQueueWithLock::GetTask(void)
{
	/// Retrieve the next JTaskBase from this queue. Upon success, a shared pointer to
	/// a JTaskBase object is returned (ownership is then considered transferred
	/// to the caller). A nullptr pointer is returned if the queue is empty or
	/// the call is interrupted. This operates without locks.	

	std::lock_guard<std::mutex> sLock(mQueueLock);
	if(mQueue.empty())
		return nullptr;
	auto sTask = std::move(mQueue.front());
	mQueue.pop_front();
	mTasksProcessed++;

	return sTask;
}

//---------------------------------
// GetNumTasks
//---------------------------------
uint32_t JQueueWithLock::GetNumTasks(void)
{
	/// Returns the number of tasks currently in this queue.
	/// n.b. this is an estimate since no lock is used
	return mQueue.size();
}

//---------------------------------
// GetNumTasksWithLock
//---------------------------------
uint32_t JQueueWithLock::GetNumTasksWithLock(void)
{
	/// Returns the number of tasks currently in this queue.
	std::lock_guard<std::mutex> sLock(mQueueLock);
	return mQueue.size();
}

//---------------------------------
// GetNumTasksProcessed
//---------------------------------
uint64_t JQueueWithLock::GetNumTasksProcessed(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumTasks for that).
	
	return mTasksProcessed;
}

//---------------------------------
// GetTaskBufferSize
//---------------------------------
std::size_t JQueueWithLock::GetTaskBufferSize(void)
{
	return mTaskBufferSize;
}

//---------------------------------
// AreEnoughTasksBuffered
//---------------------------------
bool JQueueWithLock::AreEnoughTasksBuffered(void)
{
	//This function is only called for the Event queue
	//If not enough tasks (events) are buffered, we will read in more events
	//std::cout << "num tasks, buffer size = " << GetNumTasks() << ", " << mTaskBufferSize << std::endl;
	return (mTaskBufferSize == 0) ? (GetNumTasks() >= 1) : (GetNumTasks() >= mTaskBufferSize);
}

//---------------------------------
// Clone
//---------------------------------
JQueue* JQueueWithLock::CloneEmpty(void) const
{
	//Create an empty clone of the queue (no tasks copied)
	return (new JQueueWithLock(*this));
}
