//
//    File: JQueueSet.h
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
#ifndef _JQueueSet_h_
#define _JQueueSet_h_

#include <map>
#include <vector>
#include <string>

#include "JQueueInterface.h"

class JQueueSet
{
	//If you want individual queues to skip-until-buffered (e.g. output) or skip-if-buffered (e.g. input): Is responsibility of individual JQueue

	public:

		//In order of execution priority. For example:
		//Could be I/O bound:
			//If not enough events buffered (in queue) from current input, buffer another event
			//If too many events in output buffers (queues), execute output tasks
		//Otherwise:
			//Process tasks in user-provided queues first (finish analyzing an open event)
			//Process events (Execute all processors on new event)

		enum class JQueueType { Input = 0, Output, SubTasks, Events };

		JQueueInterface* Get_Queue(JQueueType aQueueType, const std::string& aName = "") const;

		void Set_Queues(JQueueType aQueueType, const std::vector<JQueueInterface*>& aQueues);
		void Add_Queue(JQueueType aQueueType, JQueueInterface* aQueue);
		void Remove_Queues(JQueueType aQueueType);
		void Remove_Queues(void);

		std::pair<JQueueType, JTaskBase*> Get_Task(void) const;
		JTaskBase* Get_Task(JQueueType aQueueType, const std::string& aQueueName) const;

	private:
		std::map<JQueueType, std::vector<JQueueInterface*>> mQueues;
};

inline void JQueueSet::Set_Queues(JQueueSet::JQueueType aQueueType, const std::vector<JQueueInterface*>& aQueues)
{
	mQueues.emplace(aQueueType, aQueues);
}

inline void JQueueSet::Add_Queue(JQueueSet::JQueueType aQueueType, JQueueInterface* aQueue)
{
	mQueues[aQueueType].push_back(aQueue);
}

inline void JQueueSet::Remove_Queues(JQueueSet::JQueueType aQueueType)
{
	mQueues.erase(aQueueType);
}

inline void JQueueSet::Remove_Queues(void)
{
	mQueues.clear();
}

inline JQueueInterface* JQueueSet::Get_Queue(JQueueSet::JQueueType aQueueType, const std::string& aName) const
{
	auto sMapIterator = mQueues.find(aQueueType);
	if(sMapIterator == std::end(mQueues))
		return nullptr;

	const auto& sQueues = sMapIterator->second;
	if((sQueues.size() == 1) || (aName == ""))
		return sQueues[0];

	auto sFindQueue = [aName](const JQueueInterface* aQueue) -> bool { return (aQueue->GetName() == aName); };

	auto sEnd = std::end(sQueues);
	auto sVectorIterator = std::find_if(std::begin(sQueues), sEnd, sFindQueue);
	return (sVectorIterator != sEnd) ? (*sVectorIterator) : nullptr;
}

inline std::pair<JQueueSet::JQueueType, JTaskBase*> JQueueSet::Get_Task(void) const
{
	for(auto& sQueuePair : mQueues)
	{
		auto& sQueueVector = sQueuePair.second;
		for(auto& sQueue : sQueueVector)
		{
			auto sTask = sQueue->GetTask();
			if(sTask != nullptr)
				return std::make_pair(sQueuePair.first, sTask);
		}
	}
	return std::make_pair(JQueueType::Input, nullptr);
}

inline JTaskBase* JQueueSet::Get_Task(JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sQueue = Get_Queue(aQueueType, aQueueName);
	return sQueue->GetTask();
}

#endif // _JQueueSet_h_
