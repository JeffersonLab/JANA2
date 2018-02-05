//
//    File: JQueueSet.cc
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

#include "JQueueSet.h"
#include "JLog.h"

#include <algorithm>

//---------------------------------
// ~JQueueSet
//---------------------------------
JQueueSet::~JQueueSet(void)
{
	//Loop through queue types
	for(auto& sQueuePair : mQueues)
	{
		//Loop through queues of this type
		auto& sQueueVector = sQueuePair.second;
		for(auto& sQueue : sQueueVector)
			delete sQueue;
	}
}

//---------------------------------
// GetNumQueues
//---------------------------------
std::size_t JQueueSet::GetNumQueues(void) const
{
	std::size_t sNumQueues = 0;
	for(auto& sQueuePair : mQueues)
		sNumQueues += sQueuePair.second.size();
	return sNumQueues;
}

//---------------------------------
// SetQueues
//---------------------------------
void JQueueSet::SetQueues(JQueueSet::JQueueType aQueueType, const std::vector<JQueueInterface*>& aQueues)
{
	mQueues.emplace(aQueueType, aQueues);
}

//---------------------------------
// AddQueue
//---------------------------------
void JQueueSet::AddQueue(JQueueSet::JQueueType aQueueType, JQueueInterface* aQueue)
{
	mQueues[aQueueType].push_back(aQueue);
}

//---------------------------------
// RemoveQueues
//---------------------------------
void JQueueSet::RemoveQueues(JQueueSet::JQueueType aQueueType)
{
	mQueues.erase(aQueueType);
}

//---------------------------------
// RemoveQueues
//---------------------------------
void JQueueSet::RemoveQueues(void)
{
	mQueues.clear();
}

//---------------------------------
// Clone
//---------------------------------
JQueueSet* JQueueSet::Clone(void) const
{
	auto sClonedSet = new JQueueSet();

	//Loop through queue types
	for(auto& sQueuePair : mQueues)
	{
		auto sQueueType = sQueuePair.first;
		auto& sQueueVector = sQueuePair.second;

		//Loop through queues of this type
		for(auto& sQueue : sQueueVector)
		{
			auto sClonedQueue = sQueue->CloneEmpty();
			sClonedSet->AddQueue(sQueueType, sClonedQueue);
		}
	}

	return sClonedSet;
}

//---------------------------------
// GetQueues
//---------------------------------
void JQueueSet::GetQueues(std::map<JQueueSet::JQueueType, std::vector<JQueueInterface*>>& aQueues) const
{
	aQueues = mQueues;
}

//---------------------------------
// GetQueue
//---------------------------------
JQueueInterface* JQueueSet::GetQueue(JQueueSet::JQueueType aQueueType, const std::string& aName) const
{
	//Check if any queues of the desired type
	auto sMapIterator = mQueues.find(aQueueType);
	if(sMapIterator == std::end(mQueues))
		return nullptr; //nope

	//If only one queue, or name is "" (any), get the first queue
	const auto& sQueues = sMapIterator->second;
	if((sQueues.size() == 1) || (aName == ""))
		return sQueues[0];

	//Try to find the queue with the given name
	auto sFindQueue = [aName](const JQueueInterface* aQueue) -> bool { return (aQueue->GetName() == aName); };

	auto sEnd = std::end(sQueues);
	auto sVectorIterator = std::find_if(std::begin(sQueues), sEnd, sFindQueue);
	return (sVectorIterator != sEnd) ? (*sVectorIterator) : nullptr;
}

//---------------------------------
// GetTask
//---------------------------------
std::pair<JQueueSet::JQueueType, std::shared_ptr<JTaskBase>> JQueueSet::GetTask(void) const
{
	//Loop through queue types (in order of priority)
	for(auto& sQueuePair : mQueues)
	{
		auto sQueueType = sQueuePair.first;
		auto& sQueueVector = sQueuePair.second;

		//Loop through queues of this type
		for(auto& sQueue : sQueueVector)
		{
			//Get task if any
			auto sTask = sQueue->GetTask();
			if(sTask != nullptr)
				return std::make_pair(sQueueType, std::move(sTask));
		}
	}

	//No tasks remaining
	return std::make_pair(JQueueType::Events, std::shared_ptr<JTaskBase>(nullptr));
}

//---------------------------------
// GetTask
//---------------------------------
std::shared_ptr<JTaskBase> JQueueSet::GetTask(JQueueSet::JQueueType aQueueType, const std::string& aQueueName) const
{
	auto sQueue = GetQueue(aQueueType, aQueueName);
	return sQueue->GetTask();
}
