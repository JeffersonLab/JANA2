

#ifndef _JEVENTPROCESSOR_EXAMPLE2_H_
#define _JEVENTPROCESSOR_EXAMPLE2_H_

#include <iostream>
#include <mutex>
#include <JANA/JEventProcessor.h>

#include "MyCluster.h"
#include "MyHit.h"

class JEventProcessor_example2:public JEventProcessor{
	
	protected:
		std::mutex mymutex;

	public:

		//----------------------------
		// Init
		//
		// This is called once before the first call to the Process method
		// below. You may, for example, want to open an output file here.
		// Only one thread will call this.
		void Init(void){
			
		}

		//----------------------------
		// Process
		//
		// This is called for every event. Multiple threads may call this
		// simultaneously. If you write something to an output file here
		// then make sure to protect it with a mutex or similar mechanism.
		// Minimize what is done while locked since that directly affects
		// the multi-threaded performance.
		void Process(const std::shared_ptr<const JEvent>& aEvent){
			
			// Get data objects for this event. Ordering doesn't matter.
			// (n.b. Don't use a lock here!)
			auto clusters = aEvent->Get<MyCluster>();
			auto hits     = aEvent->Get<MyHit>();
			
			//--------------------------------------------------------------
			// ---- Do any potentially CPU intensive calculations here  ----
			//--------------------------------------------------------------
			
			// If your application needs to do something serially, like write
			// to a file, then use a lock to do that here.
			std::lock_guard<std::mutex> lck(mymutex);
			
			//--------------------------------------------------------------
			// -------------- Do any serial operations here  ---------------
			//--------------------------------------------------------------

		}

		//----------------------------
		// Finish
		//
		// This is called once after all events have been processed. You may,
		// for example, want to close an output file here.
		// Only one thread will call this.
		void Finish(void){

		}

};

#endif   // _JEVENTPROCESSOR_EXAMPLE2_H_

