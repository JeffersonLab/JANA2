//
//    File: JEventSource.cc
// Created: Thu Oct 12 08:15:39 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
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

#include <cxxabi.h>

#include "JEventSource.h"
#include "JFunctions.h"
#include "JApplication.h"
#include <JANA/JEvent.h>

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(string name, JApplication* aApplication) : mApplication(aApplication), mName(name)
{
	/// Do not open the source here!
	/// It will be opened in the Open() method
	///
	/// If anything more than the default JEventQueue structure is needed then create it here.
	/// You may also add JFactoryGenerator objects to the JApplication to generate factories
	/// for holding object types created by this source.
}

//---------------------------------
// ~JEventSource    (Destructor)
//---------------------------------
JEventSource::~JEventSource()
{

}

//---------------------------------
// Open
//---------------------------------
void JEventSource::Open(void)
{

}

//---------------------------------
// SetJApplication
//---------------------------------
void JEventSource::SetJApplication(JApplication *app)
{
	mApplication = app;
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
std::size_t JEventSource::GetNumEventsProcessed(void) const
{
	/// Return total number of events processed by this source.
	/// This is a little tricky. A source may consist of multiple
	/// queues that expand one "event" into many. This returns
	/// the count returned by the GetNumEventsProcessed() of the
	/// queue pointed to by the mEventQueue member of this source
	/// if it is set. This presumably is the last queue in the
	/// chain and what the caller is most interested in. If that
	/// is not set, then the value of the member mEventsProcessed
	/// is returned. This is a count of the number of events
	/// processed through all queues, including any internal ones.
	/// In those situations, this will likely be double counting.
	///
	/// Note that in the first case, the number is how many events
	/// were removed from the last queue, but some of those may
	/// still be processing. Thus, it might overcount how many were
	/// actually completed. This is only a problem if this is called
	/// while event processing is still ongoing.
	if( mEventQueue != nullptr ) return mEventQueue->GetNumTasksProcessed();

	return mEventsProcessed;
}

//---------------------------------
// GetType
//---------------------------------
std::string JEventSource::GetType(void) const
{
	return GetDemangledName<decltype(*this)>();
}

//---------------------------------
// GetProcessEventTasks
//---------------------------------
std::vector<std::shared_ptr<JTaskBase> > JEventSource::GetProcessEventTasks(std::size_t aNumTasks)
{
	/// This version is called by JThread.
	/// This will attempt to read aNumTasks from the source, wrapping each in a JTask
	/// so it can be added to this source's Event queue.

	//If file closed, return dummy pair
	if(mExhausted) return std::vector<std::shared_ptr<JTaskBase>>();

	// Optionally limit number of events read from this source
	if( (mMaxEventsToRead!=0) && ((mEventsRead + aNumTasks)>mMaxEventsToRead) ){
		aNumTasks = mMaxEventsToRead - mEventsRead;
	}

	//Initialize things before locking
	std::vector<std::shared_ptr<JEvent>> sEvents;
	sEvents.reserve(aNumTasks);

	//Attempt to acquire atomic lock
	bool sExpected = false;
	if(!mGettingEvent.compare_exchange_strong(sExpected, true)){
		// failed to get lock. Return empty container
		return std::vector<std::shared_ptr<JTaskBase> >();
	}

	//Lock aquired, get the events
	for(std::size_t si = 0; si < aNumTasks; si++)
	{
		// Read an event from the source. Anything other than a successful read
		// throws an exception (we never need to check for kSUCCESS)
		try{
			auto jevent = std::make_shared<JEvent>(mApplication);
			jevent->SetJEventSource(this);
			jevent->SetJApplication(mApplication);
			jevent->SetFactorySet(mApplication->GetFactorySet());
			GetEvent(jevent);
			sEvents.push_back(std::move(jevent));
		}
		catch(RETURN_STATUS ret_status){
			switch(ret_status){
				case RETURN_STATUS::kNO_MORE_EVENTS:
				case RETURN_STATUS::kERROR:
					mExhausted = true;
					break;
				case RETURN_STATUS::kBUSY:
				case RETURN_STATUS::kTRY_AGAIN:
				default:
					break;
			}
			break; // exception caught so don't try reading any more events right now
		}catch(...){
			mExhausted = true;
			break; // un-expected exception caught
		}
	}

	//Done with the lock: Unlock
	mGettingEvent = false;

	//Make tasks for analyzing the events
	std::vector<std::shared_ptr<JTaskBase> > sTasks;
	mEventsRead.fetch_add(sEvents.size());
	for(auto& sEvent : sEvents) sTasks.push_back(GetProcessEventTask(std::move(sEvent)));
	mTasksCreated.fetch_add(sTasks.size());


	//Return the tasks
	return sTasks;
}

//---------------------------------
// SetNumEventsToGetAtOnce
//---------------------------------
void JEventSource::SetNumEventsToGetAtOnce(std::size_t aMinNumEvents, std::size_t aMaxNumEvents)
{
	mMinNumEventsToGetAtOnce = aMinNumEvents;
	mMaxNumEventsToGetAtOnce = aMaxNumEvents;
}

//---------------------------------
// GetNumEventsToGetAtOnce
//---------------------------------
std::pair<std::size_t, std::size_t> JEventSource::GetNumEventsToGetAtOnce(void) const
{
	return std::make_pair(mMinNumEventsToGetAtOnce, mMaxNumEventsToGetAtOnce);
}

//---------------------------------
// GetProcessEventTask
//---------------------------------
std::shared_ptr<JTaskBase> JEventSource::GetProcessEventTask(std::shared_ptr<const JEvent>&& aEvent)
{
	//This version creates the task (default: run the processors), and can be overridden in derived classes (but cannot be called)
	return JMakeAnalyzeEventTask(std::move(aEvent), mApplication);
}

//---------------------------------
// IsExhausted
//---------------------------------
bool JEventSource::IsExhausted(void) const
{
	return mExhausted;
}



