// $Id$
//
//    File: JEventProcessor_jana_test.h
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventProcessor_jana_test_
#define _JEventProcessor_jana_test_

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JEventProcessor_jana_test:public JEventProcessor{
	public:
		JEventProcessor_jana_test();
		~JEventProcessor_jana_test();
		const char* className(void){return "JEventProcessor_jana_test";}

	private:
		void init(void);						///< Called once at program start.
		void brun(JEvent *jevent, int32_t runnumber);	///< Called everytime a new run number is detected.
		void evnt(JEvent *jevent, uint64_t eventnumber);	///< Called every event.
		void erun(void);						///< Called everytime run number changes, provided brun has been called.
		void fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _JEventProcessor_jana_test_

