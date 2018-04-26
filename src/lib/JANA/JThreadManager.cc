//
//    File: JThreadManager.cc
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

#include <algorithm>
#include <unistd.h>
#include <tuple>

#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif

#include "JThreadManager.h"
#include "JEventSource.h"
#include "JApplication.h"
#include "JEventSourceManager.h"
#include "JQueue.h"
#include "JEvent.h"
#include "JThread.h"

#include <pthread.h>
#include <signal.h>

//---------------------------------
// JThreadManager
//---------------------------------
JThreadManager::JThreadManager(JApplication* aApplication) : mApplication(aApplication), mEventSourceManager(mApplication->GetJEventSourceManager())
{
	gPARMS->SetDefaultParameter("JANA:THREAD_DEBUG_LEVEL", mDebugLevel, "JThread(Manager) debug level"); //Doesn't work??
	gPARMS->SetDefaultParameter("JANA:THREAD_ROTATE_SOURCES", mRotateEventSources, "Should threads rotate between event sources?");
//	mDebugLevel = 500;

	auto sSleepTimeNanoseconds = mSleepTime.count();
	gPARMS->SetDefaultParameter("JANA:THREAD_SLEEP_TIME_NS", sSleepTimeNanoseconds, "Thread sleep time (in nanoseconds) when nothing to do.");
	if(sSleepTimeNanoseconds != mSleepTime.count())
		mSleepTime = std::chrono::nanoseconds(sSleepTimeNanoseconds);
}

//---------------------------------
// ~JThreadManager
//---------------------------------
JThreadManager::~JThreadManager(void)
{
	EndThreads();
	JoinThreads();
	for(auto sThread : mThreads)
		delete sThread;
}

//---------------------------------
// CreateThreads
//---------------------------------
void JThreadManager::CreateThreads(std::size_t aNumThreads)
{
	//Create the threads, assigning them queue sets (and event sources)
	//Before calling this, the queues (& event sources) should be prepared first!
	mThreads.reserve(aNumThreads);
	for(decltype(aNumThreads) si = 0; si < aNumThreads; si++)
	{
		auto sQueueSetIndex = si % mActiveSourceInfos.size();
		mThreads.push_back(new JThread(si + 1, mApplication, mActiveSourceInfos[sQueueSetIndex], sQueueSetIndex, mRotateEventSources));
	}
}

//---------------------------------
// AddThread
//---------------------------------
void JThreadManager::AddThread(void)
{
	//Find a non-null event source info
	std::size_t sSourceIndex = 0;
	auto sEventSourceInfo = GetNextSourceQueues(sSourceIndex);

	if(sEventSourceInfo == nullptr)
		return; //Weird. We must be ending the program. Or starting it. Don't add threads this way in these situations.

	//LOCK
	LockThreadPool();

	mThreads.push_back(new JThread(mThreads.size() + 1, mApplication, sEventSourceInfo, sSourceIndex, mRotateEventSources));

	//UNLOCK
	mThreadPoolLock = false;
}

//---------------------------------
// RemoveThread
//---------------------------------
void JThreadManager::RemoveThread(void)
{
	//LOCK
	LockThreadPool();

	//Remove a thread (from the back of the pool)
	auto sThread = mThreads.back();
	mThreads.pop_back();

	//UNLOCK
	mThreadPoolLock = false;

	//End, join, and delete thread
	sThread->End();
	sThread->Join();
	delete sThread;
}

//---------------------------------
// PrepareQueues
//---------------------------------
void JThreadManager::PrepareQueues(void)
{
	//Call while single-threaded during program startup
	//For each open event source, create a set of queues using the template queue set
	//Also, add the custom event queue that is unique to each event source
	std::vector<JEventSource*> sSources;
	mEventSourceManager->GetActiveJEventSources(sSources);

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "#sources: " << sSources.size() << "\n" << JLogEnd();
	for(auto sSource : sSources)
		mActiveSourceInfos.push_back(new JEventSourceInfo(sSource, MakeQueueSet(sSource)));

	if(sSources.size() < 2)
		mRotateEventSources = false; //nothing to rotate to
}

//---------------------------------
// MakeQueueSet
//---------------------------------
JQueueSet* JThreadManager::MakeQueueSet(JEventSource* sEventSource)
{
	//Create queue set from template
	//The template contains queues that are identical for all sources (e.g. subtask, output queues)

	//Don't just use default copy-constructor.
	//We want each event source to have its own set of queues.
	//This is in case there are "barrier" events:
		//If a barrier event comes through, we don't want to analyze any
		//subsequent events until it is finished being analyzed.
		//However, we can still analyze events from other sources,
		//and don't want to block those: Separate queues
	auto sQueueSet = mTemplateQueueSet.Clone();

	//get source-specific event queue (e.g. disentangle events)
	auto sEventQueue = sEventSource->GetEventQueue();
	if(sEventQueue == nullptr) //unspecified by source, use default
		sEventQueue = new JQueue("Events");
	sQueueSet->AddQueue(JQueueSet::JQueueType::Events, sEventQueue);

	return sQueueSet;
}

//---------------------------------
// GetNJThreads
//---------------------------------
uint32_t JThreadManager::GetNJThreads(void)
{
	/// Returns the number of JThread objects that currently
	/// exist.

	return mThreads.size();
}

//---------------------------------
// GetJThreads
//---------------------------------
void JThreadManager::GetJThreads(std::vector<JThread*>& aThreads) const
{
	/// Copy list of the pointers to all JThread objects into
	/// provided container.

	aThreads = mThreads;
}

//---------------------------------
// LockScourceInfos
//---------------------------------
void JThreadManager::LockScourceInfos(void) const
{
	bool sExpected = false;
	while(!mScourceInfosLock.compare_exchange_weak(sExpected, true))
		sExpected = false;
}

//---------------------------------
// LockThreadPool
//---------------------------------
void JThreadManager::LockThreadPool(void) const
{
	bool sExpected = false;
	while(!mThreadPoolLock.compare_exchange_weak(sExpected, true))
		sExpected = false;
}

//---------------------------------
// GetQueue
//---------------------------------
JQueueInterface* JThreadManager::GetQueue(const std::shared_ptr<JTaskBase>& aTask, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sEventSource = aTask->GetEvent()->GetEventSource();
	return GetQueue(sEventSource, aQueueType, aQueueName);
}

//---------------------------------
// GetQueue
//---------------------------------
JQueueInterface* JThreadManager::GetQueue(const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sQueueSet = GetEventSourceInfo(aEventSource)->mQueueSet;
	return sQueueSet->GetQueue(aQueueType, aQueueName);
}

//---------------------------------
// GetRetiredQueues
//---------------------------------
void JThreadManager::GetRetiredSourceInfos(std::vector<JEventSourceInfo*>& aSourceInfos) const
{
	//LOCK
	LockScourceInfos();

	aSourceInfos = mRetiredSourceInfos;

	//UNLOCK
	mScourceInfosLock = false;
}

//---------------------------------
// GetActiveQueues
//---------------------------------
void JThreadManager::GetActiveSourceInfos(std::vector<JEventSourceInfo*>& aSourceInfos) const
{
	//LOCK
	LockScourceInfos();

	aSourceInfos = mActiveSourceInfos;

	//UNLOCK
	mScourceInfosLock = false;

	//filter out nullptr queues
	auto sNullChecker = [](const JEventSourceInfo* sInfo) -> bool {return (sInfo == nullptr);};
	aSourceInfos.erase(std::remove_if(std::begin(aSourceInfos), std::end(aSourceInfos), sNullChecker), std::end(aSourceInfos));
}

//---------------------------------
// SetQueue
//---------------------------------
void JThreadManager::AddQueue(JQueueSet::JQueueType aQueueType, JQueueInterface* aQueue)
{
	//Set a template queue that will be used (cloned) for each event source.
	//Doesn't lock: Assumes no one is crazy enough to call this while in the middle of running.
	//This should probably just be called in plugin constructor.
	mTemplateQueueSet.AddQueue(aQueueType, aQueue);
}

//---------------------------------
// GetEventSourceInfo
//---------------------------------
JThreadManager::JEventSourceInfo* JThreadManager::GetEventSourceInfo(const JEventSource* aEventSource) const
{
	auto sFind_Key = [aEventSource](const JEventSourceInfo* aSourceInfo) -> bool
	{
		if(aSourceInfo == nullptr)
			return false;
		return (aSourceInfo->mEventSource == aEventSource);
	};

	//LOCK
	LockScourceInfos();

	auto sEnd = std::end(mActiveSourceInfos);
	auto sIterator = std::find_if(std::begin(mActiveSourceInfos), sEnd, sFind_Key);
	auto sResult = (sIterator != sEnd) ? *sIterator : nullptr;

	//UNLOCK
	mScourceInfosLock = false;

	return sResult;
}

//---------------------------------
// GetNextQueueSet
//---------------------------------
JThreadManager::JEventSourceInfo* JThreadManager::GetNextSourceQueues(std::size_t& aInputSetIndex)
{
	//Rotate to the next event source (and corresponding queue set)
	//Increment the current set index, and if past the end, loop back to 0
	auto aCurrentSetIndex = aInputSetIndex + 1;

	//LOCK
	LockScourceInfos();

	while(true)
	{
		//If index is too large, loop back to zero
		if(aCurrentSetIndex >= mActiveSourceInfos.size())
			aCurrentSetIndex = 0;

		if(aCurrentSetIndex == aInputSetIndex)
		{
			//every source is nullptr, or there is only 1 open source
			auto sInfo = mActiveSourceInfos[aCurrentSetIndex];
			mScourceInfosLock = false; //UNLOCK
			return sInfo;
		}

		//If null, don't grab
		if(mActiveSourceInfos[aCurrentSetIndex] == nullptr)
			aCurrentSetIndex++;
		else
			break;
	}
	auto sInfo = mActiveSourceInfos[aCurrentSetIndex];

	//UNLOCK
	mScourceInfosLock = false;

	aInputSetIndex = aCurrentSetIndex;
	return sInfo;
}

//---------------------------------
// RegisterSourceFinished
//---------------------------------
JThreadManager::JEventSourceInfo* JThreadManager::RegisterSourceFinished(const JEventSource* aFinishedEventSource, std::size_t& aQueueSetIndex)
{
	//Open the next source (if not done already)
	//If two threads reach here at the same time, only one will open the source.
	//The other thread will update to the new source later below
	JEventSource::RETURN_STATUS sStatus = JEventSource::RETURN_STATUS::kUNKNOWN;
	JEventSource* sNextSource = nullptr;
	std::tie(sStatus, sNextSource) = mEventSourceManager->OpenNext(aFinishedEventSource);
	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::RegisterSourceFinished(): Source finished, next source = " << sNextSource << "\n" << JLogEnd();

	//LOCK
	LockScourceInfos();

	if(sNextSource != nullptr)
	{
		//We have just opened a new event source
		if(mDebugLevel > 0)
			JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::RegisterSourceFinished(): New source opened\n" << JLogEnd();

		//Retire the current queue set, insert a new one in its place
		mActiveSourceInfos[aQueueSetIndex]->mQueueSet->FinishedWithQueues();
		mRetiredSourceInfos.push_back(mActiveSourceInfos[aQueueSetIndex]);
		auto sInfo = new JEventSourceInfo(sNextSource, MakeQueueSet(sNextSource));
		mActiveSourceInfos[aQueueSetIndex] = sInfo;

		//Unlock and return
		mScourceInfosLock = false;
		return sInfo;
	}

	//OK, the "next" source is null. Either another thread opened a new source, or there are no more sources to open.
	if(sStatus == JEventSource::RETURN_STATUS::kUNKNOWN)
	{
		//OK, we're not quite sure what happened.
		//When trying to open the next source, we searched for the old one but it was already registered as exhausted.
		//So either:
			//Another thread opened a new source, and installed it into mActiveQueueSets at the same slot
			//There were no other sources to open, and installed nullptr in its place
		auto sInfo = mActiveSourceInfos[aQueueSetIndex];
		if(sInfo != nullptr)
		{
			//Another thread opened a new source, and installed it into mActiveQueueSets at the same slot
			if(mDebugLevel > 0)
				JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::RegisterSourceFinished(): Another thread opened a new source\n" << JLogEnd();

			//Get and return the new queue set and source (don't forget to unlock!)
			mScourceInfosLock = false;
			return sInfo;
		}

		//There were no new sources to open, so the other thread retired the queue set and put nullptr in its place

		//If done, tell all threads to end (if not done so already)
		//If not, rotate this thread to the next open source and return the next queue set
		return CheckAllSourcesDone(aQueueSetIndex);
	}

	//We removed the previous source from the active list (first one to get there), and there are no new sources to open
	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::RegisterSourceFinished(): No new sources to open\n" << JLogEnd();

	//Retire the current source info and remove it from the active queue
	//Note that we can't actually erase it: It would invalidate the queue set indices of other threads
	//Instead, we replace it with nullptr
	if(mActiveSourceInfos[aQueueSetIndex] != nullptr) //if not done already by another thread
	{
		mRetiredSourceInfos.push_back(mActiveSourceInfos[aQueueSetIndex]);
		mActiveSourceInfos[aQueueSetIndex] = nullptr;
	}

	//If done, tell all threads to end
	//If not, rotate this thread to the next open source and return the next queue set
	return CheckAllSourcesDone(aQueueSetIndex);
}

//---------------------------------
// CheckAllSourcesDone
//---------------------------------
JThreadManager::JEventSourceInfo* JThreadManager::CheckAllSourcesDone(std::size_t& aQueueSetIndex)
{
	//Only called by RegisterSourceFinished, and already within a lock on mScourceInfosLock
		//This is only called once there are no more files to open, and we want to see if all jobs are done.
	//If done, tell all threads to end
	//If not, rotate this thread to the next open source and return the next queue set

	//Check if all sources are done
	auto sNullChecker = [](const JEventSourceInfo* sInfo) -> bool {return (sInfo == nullptr);};
	if(std::all_of(std::begin(mActiveSourceInfos), std::end(mActiveSourceInfos), sNullChecker))
	{
		//The last event source is done.
		if(mDebugLevel > 0)
			JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::CheckAllSourcesDone(): All tasks from all event sources are done\n" << JLogEnd();

		//UNLOCK
		mScourceInfosLock = false;

		//Tell all threads to end.
		EndThreads();
		return nullptr;
	}

	//Not all sources are done yet.
	//Rotate this thread to the next open/task-processing source.
	do
	{
		aQueueSetIndex++;
		if(aQueueSetIndex >= mActiveSourceInfos.size())
			aQueueSetIndex = 0;
	}
	while(mActiveSourceInfos[aQueueSetIndex] == nullptr);

	//Get and return the next queue set and source (don't forget to unlock!)
	auto sInfo = mActiveSourceInfos[aQueueSetIndex];
	mScourceInfosLock = false;
	return sInfo;
}

//---------------------------------
// ExecuteTask
//---------------------------------
void JThreadManager::ExecuteTask(const std::shared_ptr<JTaskBase>& aTask, JEventSourceInfo* aSourceInfo, JQueueSet::JQueueType aQueueType)
{
	//If task is from the event queue, prepare it for execution
	//See function for details
	if(aQueueType == JQueueSet::JQueueType::Events)
		PrepareEventTask(aTask, aSourceInfo);

	try
	{
		(*aTask)(); //Execute task
	}
	catch(...)
	{
		//Catch the exception
		auto sException = std::current_exception();

		if(mDebugLevel > 0)
			JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::ExecuteTask(): Caught exception!\n" << JLogEnd();

		//Rethrow the exception
		std::rethrow_exception(sException);
	}
	if(mDebugLevel >= 10)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::ExecuteTask(): Task executed successfully.\n" << JLogEnd();
}

//---------------------------------
// PrepareEventTask
//---------------------------------
void JThreadManager::PrepareEventTask(const std::shared_ptr<JTaskBase>& aTask, const JEventSourceInfo* aSourceInfo) const
{
	//If from the event queue, add factories to the event (JFactorySet)
	//We don't add them earlier (e.g. in the event source) because it would take too much memory
	//E.g. if 200 events in the queue, we'd have 200 factories of each type
	//Instead, only the events that are actively being analyzed have factories
	//Once the JEvent goes out of scope and releases the JFactorySet, it returns to the pool

	//OK, this is a bit ugly
	//The problem is that JTask holds a shared_ptr of const JEvent*, and we need to modify it.
	//Why not just hold a non-const shared_ptr?
	//Because we don't want the user calling things like JEvent::SetEventNumber() in their factories.

	//So what do we do? The dreaded const_cast.
	auto sEvent = const_cast<JEvent*>(aTask->GetEvent());

	//Is there something else we can do instead?
	//We could pass the JFactorySet alongside the JEvent instead of as a member of it.
	//But the user is not intended to ever directly interact with the JFactorySet
	//And we'll also have to deal with this again for sending along barrier-event data.
	//This ends up requiring a lot of arguments to factory/processor/task methods.

	//Now, let's set the JFactorySet:
	//By having the JEvent Release() function recycle the JFactorySet, it doesn't have to be shared_ptr
	//If it's shared_ptr: more (slow) atomic operations
	//Just make the JFactorySet a member and cheat a little bit. It's a controlled environment.
	sEvent->SetFactorySet(mApplication->GetFactorySet());
}

//---------------------------------
// SubmitAsyncTasks
//---------------------------------
void JThreadManager::SubmitAsyncTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//Tasks are added to the specified queue.
	//Function returns as soon as all tasks are submitted; it doesn't wait until all tasks are finished.
	//Function ASSUMES all tasks from the same event source!!!

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::SubmitAsyncTasks(): Submit " << aTasks.size() << " tasks.\n" << JLogEnd();

	if(aTasks.empty())
		return;

	//Get queue
	auto sEventSource = aTasks[0]->GetEvent()->GetEventSource();
	auto sSourceInfo = GetEventSourceInfo(sEventSource);
	auto sQueueSet = sSourceInfo->mQueueSet;
	auto sQueue = sQueueSet->GetQueue(aQueueType, aQueueName);

	//Submit tasks
	SubmitTasks(aTasks, sSourceInfo, sQueue, aQueueType);
}

//---------------------------------
// SubmitTasks
//---------------------------------
void JThreadManager::SubmitTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//Tasks are added to the specified queue.
	//Function does not return until all tasks are finished.
	//Function ASSUMES all tasks from the same event source!!!
	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::SubmitTasks(): Submit " << aTasks.size() <<
		" tasks to queue type " << static_cast<std::underlying_type<JQueueSet::JQueueType>::type>(aQueueType) << ", name = " << aQueueName << ".\n" << JLogEnd();

	if(aTasks.empty())
		return;

	//Get queue
	auto sEventSource = aTasks[0]->GetEvent()->GetEventSource();
	auto sSourceInfo = GetEventSourceInfo(sEventSource);
	auto sQueueSet = sSourceInfo->mQueueSet;
	auto sQueue = sQueueSet->GetQueue(aQueueType, aQueueName);

	//Submit tasks
	SubmitTasks(aTasks, sSourceInfo, sQueue, aQueueType);

	//While waiting for results, execute tasks from the queue we submitted to
	DoWorkWhileWaitingForTasks(aTasks, sSourceInfo, sQueue, aQueueType);
}

//---------------------------------
// SubmitTasks
//---------------------------------
void JThreadManager::SubmitTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JEventSourceInfo* aSourceInfo, JQueueInterface* aQueue, JQueueSet::JQueueType aQueueType)
{
	//You would think submitting tasks would be easy.
	//However, the task JQueue can fill up, and we may not be able to put all of our tasks in.
	//If that happens, we will execute one of the tasks in the queue before trying to add more.
	//This function returns once all tasks are submitted.

	//Prepare iterators
	auto sIterator = std::begin(aTasks);
	auto sEnd = std::end(aTasks);

	//Loop over tasks and submit them
	while(sIterator != sEnd)
	{
		//Try to submit task
		if(aQueue->AddTask(*sIterator) != JQueueInterface::Flags_t::kNO_ERROR)
		{
			//Failed (queue probably full): Execute a task in the meantime
			ExecuteTask(*sIterator, aSourceInfo, aQueueType);
		}
		sIterator++; //advance to the next task
	}

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::SubmitTasks(): Tasks submitted.\n" << JLogEnd();
}

//---------------------------------
// DoWorkWhileWaiting
//---------------------------------
void JThreadManager::DoWorkWhileWaiting(const std::atomic<bool>& aWaitFlag, const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//While waiting (while flag is true), execute tasks in the specified queue.
	//Function does not return until the wait flag is false, which can only be changed by another thread.
	//This is typically called during JEvent::Get(), if the objects haven't been created yet.
	//In this case, one thread holds the input wait lock while creating them,
	//and the other thread comes here and executes other tasks until the first thread finishes.

	//The thread holding the wait lock may be submitting its own tasks (and need them to finish first),
	//so waiting threads should execute tasks from that queue to minimize the wait time.

	//Get queue for executing tasks
	auto sSourceInfo = GetEventSourceInfo(aEventSource);
	auto sQueueSet = sSourceInfo->mQueueSet;
	auto sQueue = sQueueSet->GetQueue(aQueueType, aQueueName);

	//Build is-it-done checker
	auto sWaitFunction = [&aWaitFlag](void) -> bool {return aWaitFlag;};

	//Execute tasks while waiting
	DoWorkWhileWaitingForTasks(sWaitFunction, sSourceInfo, sQueue, aQueueType);
}

//---------------------------------
// DoWorkWhileWaitingForTasks
//---------------------------------
void JThreadManager::DoWorkWhileWaitingForTasks(const std::vector<std::shared_ptr<JTaskBase>>& aSubmittedTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//The passed-in tasks have already been submitted (e.g. via SubmitTasks or SubmitAsyncTasks).
	//While waiting for those to finish, we get tasks from the queues and execute those.
	//The queue arguments should be those for the queue we just submitted the tasks to.
	//That way, this thread will help do the work that was just submitted.

	//Get queue
	auto sEventSource = aSubmittedTasks[0]->GetEvent()->GetEventSource();
	auto sSourceInfo = GetEventSourceInfo(sEventSource);
	auto sQueueSet = sSourceInfo->mQueueSet;
	auto sQueue = sQueueSet->GetQueue(aQueueType, aQueueName);

	//Defer to next function
	DoWorkWhileWaitingForTasks(aSubmittedTasks, sSourceInfo, sQueue, aQueueType);
}

//---------------------------------
// DoWorkWhileWaitingForTasks
//---------------------------------
void JThreadManager::DoWorkWhileWaitingForTasks(const std::vector<std::shared_ptr<JTaskBase>>& aSubmittedTasks, JEventSourceInfo* aSourceInfo, JQueueInterface* aQueue, JQueueSet::JQueueType aQueueType)
{
	//The passed-in tasks have already been submitted (e.g. via SubmitTasks or SubmitAsyncTasks).
	//While waiting for those to finish, we get tasks from the queues and execute those.
	//The queue arguments should be those for the queue we just submitted the tasks to.
	//That way, this thread will help do the work that was just submitted.

	//Is-it-done checker
	auto sTaskCompleteChecker = [](const std::shared_ptr<JTaskBase>& sTask) -> bool {return sTask->IsFinished();};

	//Wait if all tasks aren't finished
	//Pass in iterators instead of vector to avoid copying the vector (and ref counting)
	auto sBegin = std::begin(aSubmittedTasks);
	auto sEnd = std::end(aSubmittedTasks);
	auto sWaitFunction = [sBegin, sEnd, sTaskCompleteChecker](void) -> bool {
		return !std::all_of(sBegin, sEnd, sTaskCompleteChecker);};

	//Execute tasks while waiting
	DoWorkWhileWaitingForTasks(sWaitFunction, aSourceInfo, aQueue, aQueueType);
}

//---------------------------------
// DoWorkWhileWaitingForTasks
//---------------------------------
void JThreadManager::DoWorkWhileWaitingForTasks(const std::function<bool(void)>& aWaitFunction, JEventSourceInfo* aSourceInfo, JQueueInterface* aQueue, JQueueSet::JQueueType aQueueType)
{
	//Won't return until aWaitFunction returns true.
	//In the meantime, executes tasks (or sleeps if none available)
	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::DoWorkWhileWaitingForTasks()\n" << JLogEnd();

	while(aWaitFunction())
	{
		//Get task and execute it
		auto sQueueType = JQueueSet::JQueueType::Events;
		auto sTask = std::shared_ptr<JTaskBase>(nullptr);
		std::tie(sQueueType, sTask) = GetTask(aSourceInfo->mQueueSet, aQueue, aQueueType);
		if(sTask != nullptr)
			ExecuteTask(sTask, aSourceInfo, sQueueType);
		else
			std::this_thread::sleep_for(mSleepTime); //Sleep a minimal amount.
	}

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::DoWorkWhileWaitingForTasks(): Done waiting.\n" << JLogEnd();
}

//---------------------------------
// GetTask
//---------------------------------
std::pair<JQueueSet::JQueueType, std::shared_ptr<JTaskBase>> JThreadManager::GetTask(JQueueSet* aQueueSet, JQueueInterface* aQueue, JQueueSet::JQueueType aQueueType) const
{
	//This is called by threads who have just submitted (or are waiting on) tasks to complete,
	//and want work to do in the meantime.

	//This prefers getting tasks from the queue specified by the input arguments,
	//then searches other queues if none are there.

	//Prefer executing tasks from the input queue
	auto sTask = aQueue->GetTask();
	if(sTask != nullptr)
		return std::make_pair(aQueueType, sTask);

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::GetTask(): Queue empty.\n" << JLogEnd();

	//Next, prefer executing tasks from the same event source (input queue set)
	JQueueSet::JQueueType sQueueType;
	std::tie(sQueueType, sTask) = aQueueSet->GetTask();
	if(sTask != nullptr)
		return std::make_pair(sQueueType, sTask);

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::GetTask(): Event source tasks empty.\n" << JLogEnd();

	//Finally, loop over all active queue sets (different event sources)
	LockScourceInfos();
	for(auto sQueueSetInfo : mActiveSourceInfos)
	{
		if(sQueueSetInfo == nullptr)
			continue;
		auto sQueueSet = sQueueSetInfo->mQueueSet;
		if(sQueueSet == aQueueSet)
			continue; //already checked that one
		std::tie(sQueueType, sTask) = aQueueSet->GetTask();
		if(sTask != nullptr)
		{
			mScourceInfosLock = false; //UNLOCK
			return std::make_pair(sQueueType, sTask);
		}
	}

	if(mDebugLevel > 0)
		JLog(mLogTarget) << "Thread " << JTHREAD->GetThreadID() << " JThreadManager::GetTask(): No tasks.\n" << JLogEnd();

	//No tasks in any queues, return nullptr
	mScourceInfosLock = false; //UNLOCK
	return std::make_pair(JQueueSet::JQueueType::Events, std::shared_ptr<JTaskBase>(nullptr));
}

//---------------------------------
// SetThreadAffinity
//---------------------------------
void JThreadManager::SetThreadAffinity(int affinity_algorithm)
{
	/// Set the affinity of each thread. This will force each JThread
	/// to be assigned to a specific HW thread. At this point, it just
	/// assigns them sequentially. More sophisticated assignments should
	/// be done by the user by getting a list of JThreads and then getting
	/// a pointer to the std::thread object via the GetThread method.
	///
	/// Note that setting the thread affinity is not something that can be
	/// done through the C++ standard. It is done here only for pthreads
	/// which is the underlying thread package for Linux and Mac OS X
	/// (at least at the moment).

	// The default algorithm does not set the affinity at all
	if( affinity_algorithm==0 ){
		if(mDebugLevel > 0)
			JLog(mLogTarget) << "JThreadManager::SetThreadAffinity(): Thread affinity not set.\n" << JLogEnd();
		return;
	}

	if( typeid(std::thread::native_handle_type) != typeid(pthread_t) ){
		JLog(mLogTarget) << "\n";
		JLog(mLogTarget) << "WARNING: AFFINITY is set, but thread system is not pthreads.\n";
		JLog(mLogTarget) << "         Thread affinity will not be set by JANA.\n";
		JLog(mLogTarget) << "\n" << JLogEnd();
		return;
	}

	JLog(mLogTarget) << "JThreadManager::SetThreadAffinity(): Setting affinity for all threads using algorithm " << affinity_algorithm << "\n" << JLogEnd();

	uint32_t ithread = 0;
	uint32_t ncores = GetNcores();
	for( auto jt : mThreads ){
		pthread_t t = jt->GetThread()->native_handle();

		uint32_t icpu = ithread;
		switch( affinity_algorithm ){
			case 1:
				// This just assigns threads in numerical order.
				// This is probably the best option for most
				// production jobs.
				break;

			case 2:
				// This algorithm places subsequent threads ncores/2 apart
				// which generally will pair them up on single cores.
				// This is good if the threads benefit from sharing the L1
				// and L2 cache. It is not good if they don't since they
				// will likely compete for the core itself.
				icpu = ithread/2;
				icpu += ((ithread%2)*ncores/2);
				icpu %= ncores;
				break;
			default:
				JLog(1) << "Unknown affinity algorithm " << affinity_algorithm << "\n" << JLogEnd(); //1: std::cerr
				exit( -1 );
				break;
		}
		ithread++;

#ifdef __APPLE__
		// Mac OS X
		thread_affinity_policy_data_t policy = { (int)icpu };
		thread_port_t mach_thread = pthread_mach_thread_np( t );
		thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
		_DBG_<<"CPU: " << mApplication->GetCPU() << "  (mach_thread="<<mach_thread<<", icpu=" << icpu <<")" << _DBG_ENDL_;
#else
		// Linux
		cpu_set_t cpuset;
    	CPU_ZERO(&cpuset);
    	CPU_SET( icpu, &cpuset);
    	int rc = pthread_setaffinity_np( t, sizeof(cpu_set_t), &cpuset);
		if( rc !=0 )
			JLog(1) << "ERROR: pthread_setaffinity_np returned " << rc << " for thread " << ithread << "\n" << JLogEnd(); //1: std::cerr
#endif
	}
}

//---------------------------------
// GetNcores
//---------------------------------
uint32_t JThreadManager::GetNcores(void)
{
	/// Returns the number of cores that are on the computer.
	/// The count will be full cores+hyperthreads (or equivalent)

	return sysconf(_SC_NPROCESSORS_ONLN);
}


//---------------------------------
// RunThreads
//---------------------------------
void JThreadManager::RunThreads(void)
{
	for(auto sThread : mThreads)
		sThread->Run();
}

//---------------------------------
// EndThreads
//---------------------------------
void JThreadManager::EndThreads(void)
{
	//Tell threads to end after they finish their current tasks
	for(auto sThread : mThreads)
		sThread->End();
}

//---------------------------------
// JoinThreads
//---------------------------------
void JThreadManager::JoinThreads(void)
{
	for(auto sThread : mThreads)
		sThread->Join();
}

//---------------------------------
// JoinThreads
//---------------------------------
bool JThreadManager::HaveAllThreadsEnded(void)
{
	auto sEndChecker = [](JThread* aThread) -> bool {return aThread->IsEnded();};
	return std::all_of(std::begin(mThreads), std::end(mThreads), sEndChecker);
}
