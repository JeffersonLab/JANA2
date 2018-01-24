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

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(string name, JApplication* aApplication) : mApplication(aApplication)
{

}

//---------------------------------
// ~JEventSource    (Destructor)
//---------------------------------
JEventSource::~JEventSource()
{

}

//---------------------------------
// GetProcessEventTask
//---------------------------------
std::pair<std::shared_ptr<JTaskBase>, JEventSource::RETURN_STATUS> JEventSource::GetProcessEventTask(void)
{
	//This version is called by JThread

	//If file closed, return dummy pair
	if(mFileClosed)
		return std::make_pair(std::shared_ptr<JTaskBase>(nullptr), RETURN_STATUS::kNO_MORE_EVENTS);

	//Attempt to acquire atomic lock
	bool sExpected = false;
	if(!mInUse.compare_exchange_weak(sExpected, true)) //failed, return busy
		return std::make_pair(std::shared_ptr<JTaskBase>(nullptr), RETURN_STATUS::kBUSY);

	//Get the event from the input file
	auto sReturnStatus = RETURN_STATUS::kNO_MORE_EVENTS;
	auto sEvent = std::shared_ptr<JEvent>(nullptr);
	do
	{
		std::tie(sEvent, sReturnStatus) = GetEvent();
	}
	while((sReturnStatus == RETURN_STATUS::kTRY_AGAIN) || (sReturnStatus == RETURN_STATUS::kUNKNOWN));

	//Done with the lock: Unlock
	mInUse = false;

	//If file closed, return dummy pair
	if(sReturnStatus == RETURN_STATUS::kNO_MORE_EVENTS)
	{
		mFileClosed = true;
		return std::make_pair(std::shared_ptr<JTaskBase>(nullptr), RETURN_STATUS::kNO_MORE_EVENTS);
	}

	//Then make the task for analyzing it (default: running the processors) and return it
	return GetProcessEventTask(sEvent);
}

//---------------------------------
// GetProcessEventTask
//---------------------------------
std::pair<std::shared_ptr<JTaskBase>, JEventSource::RETURN_STATUS> JEventSource::GetProcessEventTask(std::shared_ptr<JEvent>& aEvent)
{
	//This version creates the task (default: run the processors), and can be overridden in derived classes (but cannot be called)
	return std::make_pair(JMakeAnalyzeEventTask(aEvent, mApplication), RETURN_STATUS::kSUCCESS); //From JFunctions
}

//---------------------------------
// GetEvent
//---------------------------------
std::pair<std::shared_ptr<JEvent>, JEventSource::RETURN_STATUS> JEventSource::GetEvent(void)
{
	//Get a JEvent from the file.
	//This should be overridden in derived classes.
	return std::make_pair(std::shared_ptr<JEvent>(nullptr), RETURN_STATUS::kUNKNOWN);
}

//---------------------------------
// IsDone
//---------------------------------
bool JEventSource::IsFileClosed()
{
	return mFileClosed;
}



