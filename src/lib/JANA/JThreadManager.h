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
#include <list>
#include <memory>
#include <atomic>
#include <functional>
#include <tuple>

#include "JTask.h"
#include "JQueueSet.h"
#include "JResourcePool.h"
#include "JFactorySet.h"
#include "JLog.h"

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JThread;
class JThreadManager;
class JEventSourceManager;
class JEventSource;
class JApplication;

/**************************************************************** TYPE DEFINITIONS ****************************************************************/


class JThreadManager
{
	friend JThread;

	public:

		struct JEventSourceInfo
		{
			//Struct to collect these together:
			//Each event source has a queue set

			//STRUCTORS
			JEventSourceInfo(JEventSource* aEventSource, JQueueSet* aQueueSet) : mEventSource(aEventSource), mQueueSet(aQueueSet) { }

			//MEMBERS
			JEventSource* mEventSource = nullptr;
			JQueueSet* mQueueSet = nullptr; //if no new open sources, nullptr inserted on retirement!!
		};

		//STRUCTORS
		JThreadManager(JApplication* aApplication);
		~JThreadManager(void);

		//INFORMATION
		uint32_t GetNJThreads(void);
		uint32_t GetNcores(void);

		//GETTERS
		void GetJThreads(std::vector<JThread*>& aThreads) const;

		//QUEUES
		JQueue* GetQueue(const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;
		void AddQueue(JQueueSet::JQueueType aQueueType, JQueue* aQueue);
		void PrepareQueues(void);
		void GetRetiredSourceInfos(std::vector<JEventSourceInfo*>& aSourceInfos) const;
		void GetActiveSourceInfos(std::vector<JEventSourceInfo*>& aSourceInfos) const;

		//CONFIG
		void SetThreadAffinity(int affinity_algorithm);

		//CREATE/DESTROY
		void CreateThreads(std::size_t aNumThreads);
		void RunThreads(void);
		void StopThreads(bool wait_until_idle = false);
		void EndThreads(void);
		void JoinThreads(void);
		bool AreAllThreadsIdle(void);
		bool AreAllThreadsRunning(void);
		bool AreAllThreadsEnded(void);
		void WaitUntilAllThreadsIdle(void);
		void WaitUntilAllThreadsRunning(void);
		void WaitUntilAllThreadsEnded(void);
		void AddThread(void);
		void RemoveThread(void);
		void SetNJThreads(std::size_t nthreads);

		// SUBMIT / EXECUTE TASKS
		void SubmitTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType = JQueueSet::JQueueType::SubTasks, const std::string& aQueueName = "");
		void SubmitAsyncTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType = JQueueSet::JQueueType::SubTasks, const std::string& aQueueName = "");
		void DoWorkWhileWaiting(const std::atomic<bool>& aWaitFlag, const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType = JQueueSet::JQueueType::SubTasks, const std::string& aQueueName = "");
		void DoWorkWhileWaitingForTasks(const std::vector<std::shared_ptr<JTaskBase>>& aSubmittedTasks, JQueueSet::JQueueType aQueueType = JQueueSet::JQueueType::SubTasks, const std::string& aQueueName = "");

	private:

		//CALLED DIRECTLY BY JTHREAD
		JEventSourceInfo* GetNextSourceQueues(std::size_t& aCurrentSetIndex);
		JEventSourceInfo* GetEventSourceInfo(const JEventSource* aEventSource) const;
		JQueue* GetQueue(const std::shared_ptr<JTaskBase>& aTask, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;
		JEventSourceInfo* RegisterSourceFinished(const JEventSource* aFinishedEventSource, std::size_t& aQueueSetIndex);
		void ExecuteTask(const std::shared_ptr<JTaskBase>& aTask, JEventSourceInfo* aSourceInfo, JQueueSet::JQueueType aQueueType);

		//INTERNAL CALLS
		JQueueSet* MakeQueueSet(JEventSource* sEventSource);
		void LockScourceInfos(void) const;
		void LockThreadPool(void) const;
		JEventSourceInfo* CheckAllSourcesDone(std::size_t& aQueueSetIndex);
		std::pair<JQueueSet::JQueueType, std::shared_ptr<JTaskBase>> GetTask(JQueueSet* aQueueSet, JQueue* aQueue, JQueueSet::JQueueType aQueueType) const;
		void SubmitTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JEventSourceInfo* aSourceInfo, JQueue* aQueue, JQueueSet::JQueueType aQueueType);
		void DoWorkWhileWaitingForTasks(const std::function<bool(void)>& aWaitFunction, JEventSourceInfo* aSourceInfo, JQueue* aQueue, JQueueSet::JQueueType aQueueType);
		void DoWorkWhileWaitingForTasks(const std::vector<std::shared_ptr<JTaskBase>>& aSubmittedTasks, JEventSourceInfo* aSourceInfo, JQueue* aQueue, JQueueSet::JQueueType aQueueType);
		void PrepareEventTask(const std::shared_ptr<JTaskBase>& aTask, const JEventSourceInfo* aSourceInfo) const;

		//CONTROL
		JApplication* mApplication;
		JEventSourceManager* mEventSourceManager = nullptr;
		bool mRotateEventSources = true;
		int mDebugLevel = 0;
		uint32_t mLogTarget = 0; //std::cout
		std::chrono::nanoseconds mSleepTime = std::chrono::nanoseconds(100);

		//THREADS
		std::vector<JThread*> mThreads;

		//QUEUES
		mutable std::atomic<bool> mScourceInfosLock{false};
		mutable std::atomic<bool> mThreadPoolLock{false}; //Lock on mThreads
		JQueueSet mTemplateQueueSet; //When a new source is opened, these queues are cloned for it //e.g. user-provided queues
		std::vector<JEventSourceInfo*> mActiveSourceInfos; //if no new open sources, nullptr inserted on retirement!!
		std::vector<JEventSourceInfo*> mRetiredSourceInfos; //source already finished
		std::list< std::shared_ptr<JQueueSet> > mAllocatedQueueSets; // for garbage collection when this is destroyed
};

#endif
