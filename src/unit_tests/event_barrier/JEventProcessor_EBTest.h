// $Id$
//
//    File: JEventProcessor_EBTest.h
// Created: Tue Jun  9 11:36:01 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventProcessor_EBTest_
#define _JEventProcessor_EBTest_

#include <JANA/JEventProcessor.h>

class JEventProcessor_EBTest:public jana::JEventProcessor{
	public:
		JEventProcessor_EBTest():Nerrors(0),in_barrier_event(false){}
		~JEventProcessor_EBTest(){}
		const char* className(void){return "JEventProcessor_EBTest";}

		uint32_t Nerrors;
		bool in_barrier_event;

	private:
		jerror_t init(void){return NOERROR;}
		jerror_t brun(jana::JEventLoop *loop, int32_t runnumber){return NOERROR;}
		jerror_t erun(void){return NOERROR;}
		jerror_t fini(void){return NOERROR;}

		jerror_t evnt(jana::JEventLoop *loop, uint64_t eventnumber){
		
			// We should never processes in parallel with
			// a barrier event.
			if(in_barrier_event) Nerrors++;

			// If this is a barrier event, then set the flag and
			// sleep a while to see if another thread tries
			// executing simulataneously
			if(loop->GetJEvent().GetSequential()){
				in_barrier_event = true;
				jout << endl << "--- barrier event ---" << endl;
				sleep(1);
				in_barrier_event = false;
			}
		
			return NOERROR;
		}
};

#endif // _JEventProcessor_EBTest_

