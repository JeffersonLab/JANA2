// $Id$
//
//    File: JEventProcessor_MyProcessor.h
// Created: Mon Dec 20 09:36:38 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#ifndef _JEventProcessor_MyProcessor_
#define _JEventProcessor_MyProcessor_

#include <JANA/JEventProcessor.h>

class JEventProcessor_MyProcessor:public jana::JEventProcessor{
	public:
		JEventProcessor_MyProcessor();
		~JEventProcessor_MyProcessor();
		const char* className(void){return "JEventProcessor_MyProcessor";}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _JEventProcessor_MyProcessor_

