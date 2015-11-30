// $Id$
//
//    File: JEventProcessor_UserReference.h
// Created: Tue Nov 26 10:49:45 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 13.0.0 i386)
//

#ifndef _JEventProcessor_UserReference_
#define _JEventProcessor_UserReference_

#include <pthread.h>

#include <set>
using namespace std;

#include <JANA/JEventProcessor.h>

class JEventProcessor_UserReference:public jana::JEventProcessor{
	public:
		JEventProcessor_UserReference();
		~JEventProcessor_UserReference();
		const char* className(void){return "JEventProcessor_UserReference";}

		  int GetTotalI(bool print_vals = false);
		float GetTotalF(bool print_vals = false);
		  int GetNumCountersI(void){return icounters.size();}
		  int GetNumCountersF(void){return fcounters.size();}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		set<int*> icounters;
		set<float*> fcounters;
		pthread_mutex_t mutex;
};

#endif // _JEventProcessor_UserReference_

