#include <algorithm>

#include "JThreadManager.h"

//---------------------------------
// JThreadManager
//---------------------------------
JThreadManager::JThreadManager(JEventSourceManager* aEventSourceManager, bool aRotateEventSources) :
mEventSourceManager(aEventSourceManager), mRotateEventSources(aRotateEventSources)
{
	//Get queue sets


}

//---------------------------------
// LaunchThreads
//---------------------------------
void JThreadManager::CreateThreads(std::size_t aNumThreads)
{
	mThreads.reserve(aNumThreads);
	std::size_t sQueueSetIndex = 0;
	for(decltype(aNumThreads) si = 0; si < aNumThreads; si++)
	{
		if(sQueueSetIndex > mQueueSetsBySource.size())
			sQueueSetIndex = 0;
		mThreads.push_back(new JThread(this, mQueueSetsBySource[sQueueSetIndex], sQueueSetIndex, nullptr));
		sQueueSetIndex++;
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
// GetQueueSet
//---------------------------------
JQueueSet* JThreadManager::GetQueueSet(const JEventSource* aEventSource) const
{
	auto sFind_Key = [aEventSource](const std::pair<JEventSource*, JQueueSet*>& aPair) -> bool { return (aPair.first == aEventSource); };

	//LOCK

	auto sEnd = std::end(mQueueSetsBySource);
	auto sIterator = std::find_if(std::begin(mQueueSetsBySource), sEnd, sFind_Key);
	return (sIterator != sEnd) ? (*sIterator).second : nullptr;
}

//---------------------------------
// GetNextQueueSet
//---------------------------------
JQueueSet* JThreadManager::GetNextQueueSet(std::size_t& aCurrentSetIndex)
{
	aCurrentSetIndex++;

	//LOCK

	if(aCurrentSetIndex > mQueueSetsBySource.size())
		aCurrentSetIndex = 0;

	return mQueueSetsBySource[aCurrentSetIndex];
}

//---------------------------------
// Submit_Tasks
//---------------------------------
void JThreadManager::Submit_Tasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//Tasks are added to the specified queue.
	//Function does not return until all tasks are finished.
	//Function ASSUMES all tasks from the same event source!!!

	if(aTasks.empty())
		return;

	//Submit tasks
	auto sQueue = Get_Queue(aTasks[0], aQueueType, aQueueName);
	for(auto sTask : aTasks)
		sQueue->AddTask(sTask); //TODO: Check return!!

	//Function to check whether tasks are complete
	auto sCompleteChecker = [](const std::shared_ptr<JTaskBase>& sTask) -> bool {return sTask->IsFinished();};

	//While waiting for results, execute tasks from the queue we submitted to
	do
	{
		auto sTask = sQueue->GetTask();
		(*sTask)(); //Execute task
	}
	while(!std::all_of(std::begin(aTasks), std::end(aTasks), sCompleteChecker)); //Exit if all tasks completed
}

//---------------------------------
// Get_Queue
//---------------------------------
JQueueInterface* JThreadManager::Get_Queue(const std::shared_ptr<JTaskBase>& aTask, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sEventSource = aTask->GetEvent()->GetEventSource();
	return Get_Queue(sEventSource, aQueueType, aQueueName);
}

//---------------------------------
// Get_Queue
//---------------------------------
JQueueInterface* JThreadManager::Get_Queue(const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sQueueSet = GetQueueSet(aEventSource);
	return sQueueSet->Get_Queue(aQueueType, aQueueName);
}

//---------------------------------
// Submit_AsyncTasks
//---------------------------------
void JThreadManager::Submit_AsyncTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName)
{
	//Tasks are added to the specified queue.
	//Function returns immediately; it doesn't wait until all tasks are finished.
	//Function ASSUMES all tasks from the same event source!!!

	if(aTasks.empty())
		return;

	//Submit tasks
	auto sQueue = Get_Queue(aTasks[0], aQueueType, aQueueName);
	for(auto sTask : aTasks)
		sQueue->AddTask(sTask); //TODO: Check return!!
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
		jout << "Thread affinity not set" << endl;
		return;
	}

	if( typeid(thread::native_handle_type) != typeid(pthread_t) ){
		jout << endl;
		jout << "WARNING: AFFINITY is set, but thread system is not pthreads." << endl;
		jout << "         Thread affinity will not be set by JANA." << endl;
		jout << endl;
		return;
	}

	jout << "Setting affinity for all threads using algorithm " << affinity_algorithm << endl;

	uint32_t ithread = 0;
	uint32_t ncores = GetNcores();
	for( auto jt : _jthreads ){
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
				jerr << "Unknown affinity algorithm " << affinity_algorithm << endl;
				exit( -1 );
				break;
		}
		ithread++;

#ifdef __APPLE__
		// Mac OS X
		thread_affinity_policy_data_t policy = { (int)icpu };
		thread_port_t mach_thread = pthread_mach_thread_np( t );
		thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
		_DBG_<<"CPU: " << GetCPU() << "  (mach_thread="<<mach_thread<<", icpu=" << icpu <<")" << endl;
#else
		// Linux
		cpu_set_t cpuset;
    	CPU_ZERO(&cpuset);
    	CPU_SET( icpu, &cpuset);
    	int rc = pthread_setaffinity_np( t, sizeof(cpu_set_t), &cpuset);
		if( rc !=0 ) jerr << "ERROR: pthread_setaffinity_np returned " << rc << " for thread " << ithread << endl;
#endif
	}
}

//---------------------------------
// Terminate_Threads
//---------------------------------
void JThreadManager::Terminate_Threads(void)
{
	//After they finish their current tasks
	if(mThreads.empty())
		return;

	//Join threads
	for(std::size_t si = 0; si < mThreads.size(); si++)
	{
		mThreads[si]->Stop(true);
		delete mThreads[si];
	}
	mThreads.clear();
}


