// $Id$
//
//    File: JEventProcessor_dummy.h
// Created: Tue Jan 22 09:43:15 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _JEventProcessor_dummy_
#define _JEventProcessor_dummy_

#include <JANA/JEventProcessor.h>

class JEventProcessor_dummy:public jana::JEventProcessor{
	public:
		JEventProcessor_dummy();
		~JEventProcessor_dummy();
		const char* className(void){return "JEventProcessor_dummy";}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _JEventProcessor_dummy_

