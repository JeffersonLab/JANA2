// $Id$
//
//    File: JPMUCounts_factory.h
// Created: Sat Jul 17 06:01:24 EDT 2010
// Creator: davidl (on Linux ifarmltest 2.6.34 x86_64)
//

#ifndef _JPMUCounts_factory_
#define _JPMUCounts_factory_

#include <JANA/JFactory.h>
#include "JPMUCounts.h"

class JPMUCounts_factory:public jana::JFactory<JPMUCounts>{
	public:
		JPMUCounts_factory(){};
		~JPMUCounts_factory(){};


	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		// Utility class to hold attributes structure and file descriptor
		// (if open).
		class pmu_event_t{
			public:
				int fd; // set to -1 if no event currently assigned
				struct perf_event_attr attr;
				
				// Constructor
				pmu_event_t(){
					fd = -1;
					memset(&attr, 0, sizeof(attr));
				}
		};

		unsigned int MAX_COUNTERS;
		vector<string> event_types;
		map<string, pmu_event_t> pmu_events;
		
		void RotateEvents(void); // change which events are being counted 
		
};

#endif // _JPMUCounts_factory_

