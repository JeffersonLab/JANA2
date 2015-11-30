// $Id$
//
//    File: JTest_factory.h
// Created: Wed Aug  8 20:52:23 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#ifndef _JTest_factory_
#define _JTest_factory_

#include <JANA/JFactory.h>
#include "JTest.h"
using namespace jana;

class JTest_factory:public JFactory<JTest>{
	public:
		JTest_factory();
		~JTest_factory(){};

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		double val1, val2;

		int governor_iterations;
};

#endif // _JTest_factory_

