// $Id$
//
//    File: jana_test_factory.h
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _jana_test_factory_
#define _jana_test_factory_

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include "jana_test.h"

class jana_test_factory:public JFactory{
	public:
		jana_test_factory(){};
		~jana_test_factory(){};


	private:
		void init(void);						///< Called once at program start.
		void brun(JEvent *jevent, int32_t runnumber);	///< Called everytime a new run number is detected.
		void evnt(JEvent *jevent, uint64_t eventnumber);	///< Called every event.
		void erun(void);						///< Called everytime run number changes, provided brun has been called.
		void fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _jana_test_factory_

