//
//    File: JQueue.cc
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

#include <JException.h>

#include "JQueue.h"

using namespace std;

//---------------------------------
// JQueue    (Constructor)
//---------------------------------
JQueue::JQueue(string name, bool run_processors):_name(name),_run_processors(run_processors),_done(false)
{
	_nevents_processed = 0;
	
	iread  = 0;
	iwrite = 0;
	iend   = 0;
}

//---------------------------------
// ~JQueue    (Destructor)
//---------------------------------
JQueue::~JQueue()
{

}

//---------------------------------
// AddConvertFromType
//---------------------------------
void JQueue::AddConvertFromType(string name)
{
	/// Add the name of a queue whose JEvent type can
	/// be converted into the type stored in this queue.
	
	_convert_from_types.insert(name);
}

//---------------------------------
// AddConvertFromTypes
//---------------------------------
void JQueue::AddConvertFromTypes(set<string> names)
{
	/// Add multiple names of queues whose JEvent types can
	/// be converted into the type stored in this queue.

	_convert_from_types.insert( names.begin(), names.end() );
}

//---------------------------------
// AddEvent
//---------------------------------
int JQueue::AddEvent(JEvent *jevent)
{
	/// Convert the given JEvent into a new JEvent subclass of the type stored
	/// by this queue. This default virtual method of the JQueue class is expected
	/// to be overridden by the subclass. This base class method will simply 
	/// throw an exception if called.
	throw JException("Generic JQueue::AddEvent called.");
}

//---------------------------------
// AddToQueue
//---------------------------------
int JQueue::AddToQueue(JEvent *jevent)
{
	/// Add the given JEvent to this queue. This will do so without locks.
	/// If the queue is full, it will return immediately with a value
	/// of JQueue::kQUEUE_FULL. Upon success, it will return JQueue::NO_ERROR.
	/// The specific subclass of JEvent that is passed in should be of the
	/// type held by this queue. If adding a different subclass that must
	/// be converted, use the AddEvent method.

	// The queue is maintained by 3 atomic indices. The goal of this
	// method is to increment both the iwrite and iend indices so
	// they point to the same slot upon exit. The JEvent pointer will
	// be copied into the slot just in front of the one these point
	// to, making it available to the GetEvent method. If it sees that
	// the queue is full, it returns immediately. (This is to give the
	// calling thread a chance to do something else or call this again.
	// for another try.)
	while(!_done){
		uint32_t idx = iwrite;
		uint32_t inext = (idx+1)%_queue.size();
		if( inext == iread ) return kQUEUE_FULL;
		if( iwrite.compare_exchange_weak(idx, inext) ){
			_queue[idx] = jevent;
			while( !_done ){
				if( iend.compare_exchange_weak(idx, inext) ) break;
			}
			break;
		}
	}
	
	return kNO_ERROR;
}

//---------------------------------
// GetConvertFromTypes
//---------------------------------
const set<string> JQueue::GetConvertFromTypes(void)
{
	/// Returns a vector of strings containing the types of
	/// JEvent that this JQueue can convert from.

	return _convert_from_types;
}

//---------------------------------
// GetMaxEvents
//---------------------------------
uint32_t JQueue::GetMaxEvents(void)
{
	/// Returns maximum number of Events queue can hold at one time.
	return _queue.size();
}

//---------------------------------
// GetName
//---------------------------------
std::string JQueue::GetName()
{
	/// Returns name of queue
	return _name;
}

//---------------------------------
// GetEvent
//---------------------------------
JEvent* JQueue::GetEvent(void)
{
	/// Retreive the next JEvent from this queue. Upon sucesss, a pointer to
	/// a JEvent object is returned (ownership is then considered transferred
	/// to the caller). A NULL pointer is returned if the queue is empty or
	/// the call is interrupted. This operates without locks.	

	while(!_done){
		uint32_t idx = iread;
		if(idx == iend) return NULL;
		JEvent *Event = _queue[idx];
		uint32_t inext = (idx+1)%_queue.size();
		if( iread.compare_exchange_weak(idx, inext) ){
			_nevents_processed++;
			return Event;
		}
	}
	
	// throw exception?
	return NULL;
}

//---------------------------------
// GetNumEvents
//---------------------------------
uint32_t JQueue::GetNumEvents(void)
{
	/// Returns the number of events currently in this queue.
	
	if(iend >= iread) return iend - iread;
	
	return _queue.size() - (iread - iend);
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
uint64_t JQueue::GetNumEventsProcessed(void)
{
	/// Returns number of events that have been taken out of this
	/// queue. Does not include events still in the queue (see
	/// GetNumEvents for that).
	
	return _nevents_processed;
}

//---------------------------------
// GetRunProcessors
//---------------------------------
bool JQueue::GetRunProcessors(void)
{
	/// Returns flag indicating whether this queue should have
	/// the event processors run on it's events. Usually, this
	/// flag will be set for only one JQueue. 
	
	return _run_processors;
}



