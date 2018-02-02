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

//---------------------------------
// JQueue    (Constructor)
//---------------------------------
JQueue::JQueue(const std::string& aName, std::size_t aQueueSize, std::size_t aTaskBufferSize) : JQueueInterface(aName), mTaskBufferSize(aTaskBufferSize)
{
	mQueue.resize(aQueueSize);
}

//---------------------------------
// JQueue    (Copy Constructor)
//---------------------------------
JQueue::JQueue(const JQueue& aQueue) : JQueueInterface(aQueue)
{
	//Assume this is called by Clone() or similar on an empty queue (ugh, can improve later)
	mTaskBufferSize = aQueue.mTaskBufferSize;
}

//---------------------------------
// operator=
//---------------------------------
JQueue& JQueue::operator=(const JQueue& aQueue)
{
	//Assume this is called by Clone() or similar on an empty queue (ugh, can improve later)
	JQueueInterface::operator=(aQueue);
	mTaskBufferSize = aQueue.mTaskBufferSize;
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

	auto sTempTask = aTask;
	return AddTask(std::move(sTempTask));
}

//---------------------------------
// AddTask
//---------------------------------
JQueueInterface::Flags_t JQueue::AddTask(std::shared_ptr<JTaskBase>&& aTask)
{
	/// Add the given JTaskBase to this queue. This will do so without locks.
	/// If the queue is full, it will return immediately with a value
	/// of JQueue::kQUEUE_FULL. Upon success, it will return JQueue::NO_ERROR.

	// The queue is maintained by 4 atomic indices. The goal of this
	// method is to increment both the iwrite and iend indices so
	// they point to the same slot upon exit. The JTaskBase pointer will
	// be copied into the slot just in front of the one these point
	// to, making it available to the GetEvent method. If it sees that
	// the queue is full, it returns immediately. (This is to give the
	// calling thread a chance to do something else or call this again.
	// for another try.)

	while(true)
	{
		uint32_t idx = iwrite;
		uint32_t inext = (idx + 1) % mQueue.size();
		if(inext == ibegin)
			return Flags_t::kQUEUE_FULL;

		//The queue is not full: iwrite is pointing to an empty slot: idx
		//If we can increment iwrite before someone else does, then we have exclusive access to slot idx
		//Why is access exclusive?:
			//Once we increment, the next writer will try to write to a following slot
			//The next writer can write to the next slot, but it can't increment iend until this thread does (so it will spin)
			//The next reader can't read past iend (which is before this slot), and we haven't incremented iend yet
			//And even if #threads > #slots, eventually the queue will be full and it won't get past the above check (ibegin isn't incrementing)

		if(iwrite.compare_exchange_weak(idx, inext))
		{
			//OK, we've claimed exclusive access to the slot. Insert the task.
			mQueue[idx] = std::move(aTask);

			//Now that we've inserted the task, indicate to readers that this slot can now be read (by incrementing iend)
			uint32_t save_idx = idx;
			while(!iend.compare_exchange_weak(idx, inext))
				idx = save_idx;
			return Flags_t::kNO_ERROR;
		}
	}

	return Flags_t::kNO_ERROR; //can never happen
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
// GetEvent
//---------------------------------
std::shared_ptr<JTaskBase> JQueue::GetTask(void)
{
	/// Retrieve the next JTaskBase from this queue. Upon success, a shared pointer to
	/// a JTaskBase object is returned (ownership is then considered transferred
	/// to the caller). A nullptr pointer is returned if the queue is empty or
	/// the call is interrupted. This operates without locks.	

	while(true)
	{
		uint32_t idx = iread;
		if(idx == iend)
			return nullptr;

		//The queue is not empty: iread is pointing to a used slot: idx
		//If we can increment iread before someone else does, then we have exclusive read access to slot idx
		//Why is access exclusive?:
			//Once we increment, the next reader will try to read a following slot
			//The next reader can read the next slot, but it can't increment ibegin until this thread does (so it will spin)
			//The next writer can't write past ibegin (which is this slot), and we haven't incremented ibegin yet
			//And even if #threads > #slots, eventually the queue will be full and it won't get past the above check (iend isn't incrementing)

		auto sTask = mQueue[idx];
		uint32_t inext = (idx + 1) % mQueue.size();
		if( iread.compare_exchange_weak(idx, inext) )
		{
			//OK, we've claimed exclusive access to the slot. Move out the task (doesn't increase reference count).
			auto sTask = std::move(mQueue[idx]);
			mTasksProcessed++;

			//Now that we've retrieved the task, indicate to writers that this slot can now be written-to (by incrementing ibegin)
			uint32_t save_idx = idx;
			while(!ibegin.compare_exchange_weak(idx, inext))
				idx = save_idx;

			return sTask; //should be copy-elided
		}
	}

	return nullptr; //can never happen
}

//---------------------------------
// GetNumTasks
//---------------------------------
uint32_t JQueue::GetNumTasks(void)
{
	/// Returns the number of tasks currently in this queue.
	//size 10, end = 2, begin = 9: -7
	auto sDifference = (int)iend - (int)ibegin;
	return (sDifference >= 0) ? sDifference : sDifference + mQueue.size();
}

//---------------------------------
// GetNumTasksProcessed
//---------------------------------
uint64_t JQueue::GetNumTasksProcessed(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumTasks for that).
	
	return mTasksProcessed;
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
JQueueInterface* JQueue::Clone(void) const
{
	return (new JQueue(*this));
}
