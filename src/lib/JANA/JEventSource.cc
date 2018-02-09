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

#include "JEventSource.h"
#include "JFunctions.h"
#include "JApplication.h"
#include <JANA/JEvent.h>

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(string name, JApplication* aApplication) : mApplication(aApplication), mName(name)
{
	//Do not open the source here!
	//It will be opened in the Open() method

	//Create JEventQueue here!
	//Create JFactoryGenerator here! (for all types in the files)
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
// GetProcessEventTask
//---------------------------------
std::pair<std::vector<std::shared_ptr<JTaskBase>>, JEventSource::RETURN_STATUS> JEventSource::GetProcessEventTasks(std::size_t aNumTasks)
{
	//This version is called by JThread

	//If file closed, return dummy pair
	if(mFileClosed)
		return std::make_pair(std::vector<std::shared_ptr<JTaskBase>>(), RETURN_STATUS::kNO_MORE_EVENTS);

	//Initialize things before locking
	std::vector<std::shared_ptr<const JEvent>> sEvents;
	sEvents.reserve(aNumTasks);
	auto sFailureStatus = JEventSource::RETURN_STATUS::kSUCCESS;

	//Attempt to acquire atomic lock
	bool sExpected = false;
	if(!mGettingEvent.compare_exchange_strong(sExpected, true)) //failed, return busy
		return std::make_pair(std::vector<std::shared_ptr<JTaskBase>>(), RETURN_STATUS::kBUSY);

	//Lock aquired, get the events
	for(std::size_t si = 0; si < aNumTasks; si++)
	{
		//Get an event from the input file
		auto sEventPair = GetEvent();

		//Save the event if we succeeded
		if(sEventPair.first != nullptr)
			sEvents.push_back(std::move(sEventPair.first));
		else //Break if there's an issue
		{
			sFailureStatus = sEventPair.second;
			break;
		}
	}

	//Done with the lock: Unlock
	mGettingEvent = false;

	//Make tasks for analyzing the events
	std::pair<std::vector<std::shared_ptr<JTaskBase>>, JEventSource::RETURN_STATUS> sTasks;
	sTasks.second = sFailureStatus; //is kSuccess if nothing went wrong
	for(auto& sEvent : sEvents)
		sTasks.first.push_back(GetProcessEventTask(std::move(sEvent)));

	//If the file is empty, note it
	if(sFailureStatus == RETURN_STATUS::kNO_MORE_EVENTS)
		mFileClosed = true;

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
// IsDone
//---------------------------------
bool JEventSource::IsFileClosed(void) const
{
	return mFileClosed;
}



