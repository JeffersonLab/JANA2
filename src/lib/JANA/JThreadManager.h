//
//    File: JThreadManager.h
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

#ifndef JThreadManager_h
#define JThreadManager_h

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "JThread.h"
#include "JTask.h"
#include "JQueueSet.h"

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JThreadManager;
class JEventSourceManager;
class JEventSource;

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

class JThreadManager
{
	friend JThread;

	public:

		//STRUCTORS
		JThreadManager(JEventSourceManager* aEventSourceManager, bool aRotateEventSources);
		~JThreadManager(void){TerminateThreads();};

		//INFORMATION
		uint32_t GetNcores(void);
		uint32_t GetNJThreads(void);

		//GETTERS
		void GetJThreads(std::vector<JThread*>& aThreads) const;
		JQueueInterface* Get_Queue(const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;

		//CONFIG
		void SetThreadAffinity(int affinity_algorithm);
		//YIKES: Must set in a way such that all future event sources get this too!!
		void SetQueue(JQueueSet::JQueueType aQueueType, JQueueInterface* aQueue, const std::string& aEventSourceGeneratorName = ""); //"" = all

		//CREATE/DESTROY
		void CreateThreads(std::size_t aNumThreads);
		void TerminateThreads(void);

		//SUBMIT
		void SubmitTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName = "");
		void SubmitAsyncTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName = "");

	private:

		JQueueSet* GetNextQueueSet(std::size_t& aCurrentSetIndex);
		JQueueSet* GetQueueSet(const JEventSource* aEventSource) const;
		JQueueInterface* Get_Queue(const std::shared_ptr<JTaskBase>& aTask, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;

		//THREADS
		std::vector<JThread*> mThreads;
		mutable std::atomic<bool> mQueueSetsLock{false};

		//faster to linear-search unsorted vector-of-pairs than sorted-vector or map (unless large (~50+) number of open sources)
		JQueueSet* mTemplateQueueSet; //When a new source is opened, these queues are cloned for it
		std::vector<std::pair<JEventSource*, JQueueSet*>> mActiveQueueSets;

		bool mRotateEventSources;
		JEventSourceManager* mEventSourceManager;
};

#endif
