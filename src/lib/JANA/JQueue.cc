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

#include <JApplication.h>
#include <JException.h>

#include "JQueue.h"

//++++++++++++++++++++++++++++++++++++++++++++++++
//#include <fstream>
//using namespace std;
//typedef struct{
//	uint32_t type;
//	uint32_t id;
//	uint32_t iwrite;
//	uint32_t iread;
//	uint32_t iend;
//	uint32_t idx;
//	uint32_t inext;
//	uint64_t ptr;
//}buff_t;
//std::atomic<int> jthread_id(0);
//thread_local uint32_t tid;
//thread_local uint32_t cnt = 0;
//thread_local buff_t* BUFF = NULL;
//
//void AddBuff(uint32_t type, uint32_t iwrite, uint32_t iread, uint32_t iend, uint32_t idx, uint32_t inext, uint64_t ptr){
//	if(cnt>=1000) return;
//	if(!BUFF) {
//		BUFF = new buff_t[1000];
//		tid = jthread_id++;
//	}
//	buff_t *buff = &BUFF[cnt++];
//	buff->type = type;
//	buff->id = tid;
//	buff->iwrite = iwrite;
//	buff->iread = iread;
//	buff->iend = iend;
//	buff->idx = idx;
//	buff->inext = inext;
//	buff->ptr = ptr;
//}
//
//// Called from JThread destructor
//void WriteBuff(void){
//	char fname[100];
//	sprintf(fname, "ringbuff_%d.out", tid);
//	ofstream ofs(fname);
//	for(int i=0; i<1000; i++){
//		buff_t *b = &BUFF[i];
//		ofs << tid << "," << b->type << "," << b->iwrite << "," << b->iread << "," << b->iend << "," << b->ptr <<endl;
//	}
//	ofs.close();
//}
//
//++++++++++++++++++++++++++++++++++++++++++++++++


using namespace std;

//---------------------------------
// JQueue    (Constructor)
//---------------------------------
JQueue::JQueue(string name, bool run_processors):_name(name),_run_processors(run_processors),_done(false),_pass_through(false),_can_sink(false)
{
	_nevents_processed = 0;
	
	iread  = 0;
	iwrite = 0;
	iend   = 0;
	
	_queue.resize(200);
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
	/// to be overridden by the subclass. As such, the default behavior here is
	/// to throw an exception. The exception may be supressed by calling 
	/// SetNoException which essentially configures the queue in "pass through"
	/// mode allowing it to accept events without any conversion.
	
	if( !_pass_through ) throw JException("Generic JQueue::AddEvent called.");
	
	AddToQueue(jevent); // only for pass through mode.

	return kNO_ERROR;
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
			uint32_t save_idx = idx;
			while( !_done ){
				if( iend.compare_exchange_weak(idx, inext) ) break;
				idx = save_idx;
			}
			break;
		}
	}

	return kNO_ERROR;
}

//---------------------------------
// GetCanSink
//---------------------------------
bool JQueue::GetCanSink(void)
{
	/// Return state of "can sink" flag.
	/// See SetCanSink() for details.

	return _can_sink;
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
		if( idx == iend ) return NULL;
		JEvent *jevent = _queue[idx];
		uint32_t inext = (idx+1)%_queue.size();
		if( iread.compare_exchange_weak(idx, inext) ){
			_nevents_processed++;
			return jevent;
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

//---------------------------------
// SetCanSink
//---------------------------------
void JQueue::SetCanSink(bool can_sink)
{
	/// Set flag indicating this queue can act as an event "sink".
	/// Being a sink means that events pulled from this queue
	/// *may* be thrown away under certain conditions. Normally, 
	/// when an event is pulled from a queue, it has event processors
	/// run on it, or it is converted into another form that fits
	/// in another queue (or both). If neither of these happens,
	/// then it is considered an error and an exception is thrown.
	/// Setting the "can sink" flag avoids the exception and quietly
	/// discards the event if no queue is found to convert from this
	/// queue's type. 
	///
	/// Note that if the "run processors" flag is set, then that
	/// also allows this to act as a sink and so setting the "can
	/// sink" flag has no effect. Likewise, if another JQueue is
	/// added that can convert events from this queue, then the
	/// events will be passed on to that regardless of the setting
	/// of the "can sink" flag. 
	///
	/// The intended purpose of this is to allow a final stage queue
	/// that does something useful with the events while converting
	/// them to go into it. (e.g. write them to a file). That would
	/// an alternative configuration to using JEventProcessors to
	/// process the events from the final queue.

	_can_sink = can_sink;
}

//---------------------------------
// SetNoException
//---------------------------------
void JQueue::SetPassThrough(bool pass_through)
{
	/// Set flag used only by default AddEvent method to allow
	/// events to be passed in without throwning an exception.
	/// The default behavior is to throw an exception when the
	/// AddEvent method of the base class is called since that
	/// will not be what is desired in most cases. Setting this
	/// flag to true will allow this JQueue base class to be used
	/// in a pass through like mode where JEvent objects can be
	/// placed into it without conversion.

	_pass_through = pass_through;
}

//---------------------------------
// SetRunProcessors
//---------------------------------
void JQueue::SetRunProcessors(bool run_processors)
{
	/// Flag this queue to run (or not) the JEventProcessors
	/// on an event as it is pulled from the queue. The most
	/// common case will have only one JQueue with this flag
	/// set to run the processors. If no arguments are given,
	/// then this flag will be set to true. Otherwise, it will
	/// be set to whatever argument is passed.

	_run_processors = run_processors;
}

