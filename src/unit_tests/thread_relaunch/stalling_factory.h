// $Id$
//
//    File: stalling_factory.h
// Created: Tue Jan 22 09:41:30 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _stalling_factory_
#define _stalling_factory_

#include <JANA/JFactory.h>
#include "stalling.h"

class stalling_factory:public jana::JFactory<stalling>{
	public:
		stalling_factory(){};
		~stalling_factory(){};


	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _stalling_factory_

