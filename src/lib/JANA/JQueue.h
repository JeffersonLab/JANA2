//
//    File: JQueue.h
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
#ifndef _JQueue_h_
#define _JQueue_h_

#include <cstdint>
#include <string>
#include <memory>

class JTaskBase;

class JQueue
{
	public:
	
		enum class Flags_t {
			kNone,
			kQUEUE_FULL,
			kNO_ERROR
		};

		//STRUCTORS
		JQueue(const std::string& aName);
		virtual ~JQueue() = default;

		//COPIERS //needed because movers specified
		JQueue(const JQueue& aQueue) = default;
		JQueue& operator=(const JQueue& aQueue) = default;

		//MOVERS //needed because destructor specified
		JQueue(JQueue&&) = default;
		JQueue& operator=(JQueue&&) = default;

		virtual Flags_t AddTask(const std::shared_ptr<JTaskBase>& aTask) = 0;
		virtual Flags_t AddTask(std::shared_ptr<JTaskBase>&& aTask) = 0;
		virtual void AddTasksProcessedOutsideQueue(std::size_t nTasks) {}
		virtual std::shared_ptr<JTaskBase> GetTask(void) = 0;
		virtual bool AreEnoughTasksBuffered(void){return true;} //Only used for the event queue

		virtual uint32_t GetMaxTasks(void) = 0;
		virtual uint32_t GetNumTasks(void) = 0;
		virtual uint64_t GetNumTasksInserted(void){ return 0.0;}
		virtual uint64_t GetNumTasksProcessed(void) = 0;
		virtual std::size_t GetTaskBufferSize(void) = 0;
		virtual std::size_t GetLatestBarrierEventUseCount(void) const{return 0;}

		virtual void FinishedWithQueue(void){} //Call this when finished with the queue

		std::string GetName(void) const;
		virtual JQueue* CloneEmpty(void) const = 0; //Create an empty clone of the queue (no tasks copied)

	private:
		std::string mName;
};

inline JQueue::JQueue(const std::string& aName) : mName(aName) { }

inline std::string JQueue::GetName(void) const
{
	return mName;
}

#endif // _JQueue_h_
