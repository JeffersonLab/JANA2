//
//    File: JQueueSimple.h
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
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JQueueSimple_h_
#define _JQueueSimple_h_

#include <cstdint>
#include <atomic>
#include <vector>

#include "JQueue.h"

class JQueueSimple : public JQueue
{
	public:
	
		JQueueSimple(const std::string& aName, std::size_t aQueueSize = 200, std::size_t aTaskBufferSize = 0);
		~JQueueSimple();

		//COPIERS //needed because atomic not copyable
		JQueueSimple(const JQueueSimple& aQueue);
		JQueueSimple& operator=(const JQueueSimple& aQueue);

		//MOVERS //specify because deleted by default if copiers specified
		JQueueSimple(JQueueSimple&&) = default;
		JQueueSimple& operator=(JQueueSimple&&) = default;

		Flags_t AddTask(const std::shared_ptr<JTaskBase>& aTask);
		Flags_t AddTask(std::shared_ptr<JTaskBase>&& aTask);
		void AddTasksProcessedOutsideQueue(std::size_t nTasks);
		std::shared_ptr<JTaskBase> GetTask(void);
		bool AreEnoughTasksBuffered(void);

		uint32_t GetMaxTasks(void);
		uint32_t GetNumTasks(void);
		uint64_t GetNumTasksInserted(void);
		uint64_t GetNumTasksProcessed(void);
		uint64_t GetNumTasksProcessedOutsideQueue(void);
		std::size_t GetTaskBufferSize(void);
	
		JQueue* CloneEmpty(void) const;

	private:

		// mTaskBufferSize is the used to hold the minimum number of events
		// needed for the the AreEnoughTasksBuffered() method to return true.
		// It is used when this queue holds "Events" from a source and JANA
		// wants to be efficient at reading them by keeping the disk spinning.
		// It is set in the constructor with a default of "0" which is the
		// equivalent to setting it to "1".
		std::size_t mTaskBufferSize = 0; //min event task buffer (only checked for Events queue) //will get more events if # tasks < this

		int mDebugLevel = 0;
		uint32_t mLogTarget = 0;

		std::vector<std::shared_ptr<JTaskBase>> mQueue;
		std::atomic<uint64_t> mTasksProcessed{0};
		std::atomic<uint64_t> mTasksInserted{0};
		std::atomic<uint64_t> mTasksRunOutsideQueue{0};

		std::size_t mNslots = 0;
		std::atomic<uint32_t> *mWriteSlotptr = nullptr;
		std::atomic<uint32_t> *mReadSlotptr = nullptr;
};

#endif // _JQueueSimple_h_
