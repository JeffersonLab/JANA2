// Author: David Lawrence  July 17, 2010
//
//

// This plugin is designed to work with the libpfm4 library
// (perfmon). It is written specifically for tests on a 48-core
// AMD machine on load to JLab for testing.

#include "JPMUCounts.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include <map>
using std::map;

class JEventProcessorJANAPFM:public JEventProcessor
{
	public:
		jerror_t init(void);							///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, int eventnumber);	///< Called every event.
		jerror_t erun(void){return NOERROR;};	///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);							///< Called after last event of last event source has been processed.

		// Class to hold counts
		class pmu_counts_t{
			public:
				uint64_t counts;
				uint64_t time_enabled;
				uint64_t time_running;
				uint64_t N; ///< number of physics events contributing to this count
				
				// Constructor
				pmu_counts_t(){
					counts = time_enabled = time_running = 0;
					N = 0;
				}
		};
		
		// Container to hold counts
		map<pair<string,pthread_t>, pmu_counts_t> pmu_counts; // key=event type

		pthread_mutex_t mutex;
};
