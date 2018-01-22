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

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

class JThreadManager
{
	friend JThread;

	public:

		//STRUCTORS
		JThreadManager(JEventSourceManager* aEventSourceManager, bool aRotateEventSources);
		~JThreadManager(void){Terminate_Threads();};

		//INFORMATION
		uint32_t GetNcores(void);
		uint32_t GetNJThreads(void);

		//GETTERS
		void GetJThreads(std::vector<JThread*>& aThreads) const;
		JQueueInterface* Get_Queue(const JEventSource* aEventSource, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;

		//CONFIG
		void SetThreadAffinity(int affinity_algorithm);
		//YIKES: Must set in a way such that all future event sources get this too!!
		void SetQueue(JQueueSet::JQueueType aQueueType, JQueueInterface* aQueue, const std::string& aEventSourceType = ""); //"" = all

		//CREATE/DESTROY
		void CreateThreads(std::size_t aNumThreads);
		void Terminate_Threads(void);

		//SUBMIT
		void Submit_Tasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName = "");
		void Submit_AsyncTasks(const std::vector<std::shared_ptr<JTaskBase>>& aTasks, JQueueSet::JQueueType aQueueType, const std::string& aQueueName = "");

	private:

		JQueueSet* GetNextQueueSet(std::size_t& aCurrentSetIndex);
		JQueueSet* GetQueueSet(const JEventSource* aEventSource) const;
		JQueueInterface* Get_Queue(const std::shared_ptr<JTaskBase>& aTask, JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const;

		//THREADS
		std::vector<JThread*> mThreads;
		mutable std::atomic<bool> mQueueSetsLock{false};

		//faster to linear-search unsorted vector-of-pairs than sorted-vector or map (unless large (~50+) number of open sources)
		std::vector<std::pair<JEventSource*, JQueueSet*>> mQueueSetsBySource;

		bool mRotateEventSources;
		JEventSourceManager* mEventSourceManager;

};

#endif
