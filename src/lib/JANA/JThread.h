//
//    File: JThread.h
// Created: Wed Oct 11 22:51:22 EDT 2017
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
#ifndef _JThread_h_
#define _JThread_h_

#include <thread>
#include <atomic>

#include "JQueueInterface.h"
#include "JQueueSet.h"
#include "JLog.h"

class JThreadManager;
class JEventSource;
class JApplication;

class JThread{
	public:

		enum RUN_STATE_t {
			kRUN_STATE_INITIALIZING,
			kRUN_STATE_IDLE,
			kRUN_STATE_RUNNING,
			kRUN_STATE_ENDED,
			kRUN_STATE_OTHER
		};

		JThread(int aThreadID, JApplication* aApplication, JQueueSet* aQueueSet, std::size_t aQueueSetIndex, JEventSource* aSource, bool aRotateEventSources);
		virtual ~JThread();

		uint64_t GetNumEventsProcessed(void);
		void GetNumEventsProcessed(std::map<std::string,uint64_t> &Nevents);
		std::thread* GetThread(void);
		int GetThreadID(void) const;
		void Join(void);

		void End(void);
		bool IsIdle(void);
		bool IsEnded(void);
		bool IsJoined(void);
		void Loop(void);
		void Loop_Body(void);
		void Run(void);
		void Stop(bool wait_until_idle = false);
		
	protected:
		
		JApplication* mApplication = nullptr;
		JThreadManager* mThreadManager = nullptr;
		JQueueSet* mQueueSet = nullptr;
		std::size_t mQueueSetIndex = 0;
		JEventSource* mEventSource = nullptr;

		JQueueInterface* mEventQueue = nullptr;
		bool mRotateEventSources = false;
		bool mSourceEmpty = false;
		std::size_t mFullRotationCheckIndex = 0; //For detecting when we are simply rotating and not executing
		std::chrono::nanoseconds mSleepTime = std::chrono::nanoseconds(100);

		int mDebugLevel = 0;
		JLog* mLogger = nullptr;

		std::thread *_thread;
		std::atomic<RUN_STATE_t> mRunState{kRUN_STATE_IDLE};           ///< Current state
		std::atomic<RUN_STATE_t> mRunStateTarget{kRUN_STATE_IDLE};    ///< State to transtion to after current event
		bool _isjoined = false;
		std::map<std::string, uint64_t> _events_processed;
		
		int mThreadID;

	private:

		//INTERNAL CALLS
		bool CheckEventQueue(void);
		void HandleNullTask(void);
};

extern thread_local JThread *JTHREAD;

#endif // _JThread_h_

