//
//    File: JThread.cc
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

#include <thread>
#include <chrono>
using namespace std;

#include <JEvent.h>
#include <JQueue.h>
#include "JThread.h"

thread_local JThread *JTHREAD = NULL;

//---------------------------------
// JThread    (Constructor)
//---------------------------------
JThread::JThread(JApplication *app):_run_state(kRUN_STATE_INITIALIZING)
{

	_run_state = kRUN_STATE_IDLE;
	_run_state_target = kRUN_STATE_IDLE;
	_isjoined = false;
	
	_thread = new thread( &JThread::Loop, this );
}

//---------------------------------
// ~JThread    (Destructor)
//---------------------------------
JThread::~JThread()
{

}

//---------------------------------
// GetNeventsProcessed
//---------------------------------
uint64_t JThread::GetNeventsProcessed(void)
{
	/// Get total number of events processed by this thread. This
	/// returns a total of all "events" from all queues. Since the
	/// nature of events from different queues differs (e.g. one
	/// event in queue "A" may turn into 100 events in queue "B")
	/// this value should be used with caution. See the other form
	/// GetNeventsProcessed(map<string,uint64_t> &) which returns
	/// counts for individual queues for a perhaps more useful breakdown.
	
	map<string,uint64_t> Nevents;
	GetNeventsProcessed(Nevents);
	
	uint64_t Ntot = 0;
	for(auto p : Nevents) Ntot += p.second;
	
	return Ntot;
}

//---------------------------------
// GetNeventsProcessed
//---------------------------------
void JThread::GetNeventsProcessed(map<string,uint64_t> &Nevents)
{
	/// Get number of events processed by this thread for each
	/// JQueue. The key will be the name of the JQueue and value
	/// the number of events from that queue processed. The returned
	/// map is not guaranteed to have an entry for each JQueue since
	/// it is possible this thread has not processed events from all
	/// JQueues.
	
	Nevents = _events_processed;
}

//---------------------------------
// GetThread
//---------------------------------
thread* JThread::GetThread(void)
{
	/// Get the C++11 thread object.

	return _thread;
}

//---------------------------------
// Join
//---------------------------------
void JThread::Join(void)
{
	/// Join this thread. If the thread is not already in the ended
	/// state then this will call End() and wait for it to do so
	/// before calling join. This should generally only be called 
	/// from a method JApplication.
	
	if( _run_state != kRUN_STATE_ENDED ) End();
	while( _run_state != kRUN_STATE_ENDED ) this_thread::sleep_for( chrono::microseconds(100) );
	_thread->join();
	_isjoined = true;
}

//---------------------------------
// End
//---------------------------------
void JThread::End(void)
{
	/// Stop the thread from processing events and end operation
	/// completely. If an event is currently being processed by the
	/// thread then that will complete first. The JThread will then
	/// exit. If you wish to stop processing of events temporarily
	/// without exiting the thread then use Stop().
	_run_state_target = kRUN_STATE_ENDED;
	
	// n.b. to implement a "wait_until_ended" here we would need
	// to do somethimg like register a flag that gets set just
	// before the thread exits since once it does, we can't actually
	// access the JThread (i.e. the "this" pointer is will cease
	// to be valid). Implementation of that is deferred until the
	// need is made clear.
}

//---------------------------------
// IsIdle
//---------------------------------
bool JThread::IsIdle(void)
{
	/// Return true if the thread is currently in the idle state.
	/// Being in the idle state means the thread is waiting to be
	/// told to start processing events (this does not mean it is
	/// waiting for an event to show up in the queue). Use this
	/// after calling Stop() to know when the thread has finished
	/// processing its current event.
	
	return _run_state == kRUN_STATE_IDLE;
}

//---------------------------------
// IsEnded
//---------------------------------
bool JThread::IsEnded(void)
{
	return _run_state == kRUN_STATE_ENDED;
}

//---------------------------------
// IsJoined
//---------------------------------
bool JThread::IsJoined(void)
{
	return _isjoined;
}

//---------------------------------
// Loop
//---------------------------------
void JThread::Loop(void)
{
	/// Loop continuously, processing events
	
	// Set thread_local global variable
	JTHREAD = this;

	while( _run_state_target != kRUN_STATE_ENDED ){
		
		// If specified, go into idle state
		if( _run_state_target == kRUN_STATE_IDLE ) _run_state = kRUN_STATE_IDLE;

		// If in the running state, try and process an event
		if(_run_state == kRUN_STATE_RUNNING){

			// Loop over queues looking for event
			// (rit_queue = "reverse iterator queue" and is queue event was pulled from)
			JEvent *event = NULL;
			auto rit_queue = _queues.rbegin();
			for(; rit_queue != _queues.rend(); rit_queue++){
				event = (*rit_queue)->GetEvent();
				if(event) break;
			}

			// Process event if found
			if(event){
				
				// This flag is used to verify that either the event 
				// processors or downstream queue converter were run
				// on this event. If nothing is done then that is
				// considered an error condition.
				bool event_processed = false;
				
				// Run processors on this event if the queue has the
				// run_processors flag set. This is where the bulk of
				// reconstruction is expected to happen.
				if( (*rit_queue)->GetRunProcessors() ) {
					
					event_processed = true;
				}
				
				// Look downstream for queue that can convert from this
				// subclass of JEvent.
				auto it_queue = rit_queue.base(); // n.b. already points to next queue!
				const string &queue_name = (*it_queue)->GetName();
				JQueue *next_queue = NULL;
				for(; it_queue != _queues.end(); it_queue++){
					// Loop over names this can convert from
					for( auto &n : (*it_queue)->GetConvertFromTypes() ){
						if( n == queue_name){
							next_queue = *it_queue;
						}
						if(next_queue) break;
					}
					if(next_queue) break;
				}
				
				// If downstream queue was found, use it to convert event
				if(next_queue) {
					next_queue->AddEvent(event);
					event_processed = true;
				}else{
					event->Recycle(); // return to pool or delete JEvent
					event = NULL; // keep a cleaner house
				}
				
				// Verify that something was done to process this event
				if( !event_processed ){
					throw JException("No queue registered that can convert from %s and this queue not flagged for event processing", queue_name.c_str());
				}
				
				// continue while loop to process next event
				continue;
			}
		}
		
		// If we get here then we either are not in the running state or
		// or there are no events to process. Sleep a minimal amount.
		this_thread::sleep_for( chrono::nanoseconds(100) );
	}
	
	// Set flag that we're done just before exiting thread
	_run_state = kRUN_STATE_ENDED;
}

//---------------------------------
// Run
//---------------------------------
void JThread::Run(void)
{
	/// Start this thread processing events. This simply
	/// sets the run state flag. 

	_run_state = _run_state_target = kRUN_STATE_RUNNING;
}

//---------------------------------
// SetQueues
//---------------------------------
void JThread::SetQueues(const vector<JQueue*> *queues)
{
	
}

//---------------------------------
// Stop
//---------------------------------
void JThread::Stop(bool wait_until_idle)
{
	/// Stop the thread from processing events. The stop will occur
	/// between events so if an event is currently being processed,
	/// that will complete before the thread goes idle. If wait_until_idle
	/// is true, then this will not return until the thread is no longer
	/// in the "running" state and therefore no longer processing events.
	/// This will usually be because the thread has gone into the idle
	/// state, but may also be because the thread has been told to end
	/// completely.
	/// Use IsIdle() to check if the thread is in the idle
	/// state.
	_run_state_target = kRUN_STATE_IDLE;
	
	// The use of yield() here will (according to stack overflow) cause
	// the core to be 100% busy while waiting for the state to change.
	// That should only be for the duration of processing of the current
	// event though and this method is expected to be used rarely so
	// this should be OK.
	if( wait_until_idle ) while( _run_state == kRUN_STATE_RUNNING ) this_thread::yield();
}
