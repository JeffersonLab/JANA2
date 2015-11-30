// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include <map>
using std::map;

class TTree;

class JEventProcessorJANARATE:public JEventProcessor
{
	public:
		jerror_t init(void);							///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);	///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);							///< Called after last event of last event source has been processed.		

		pthread_mutex_t mutex;
		bool initialized;
		bool finalized;
		int prescale;
		
		struct itimerval start_tmr;
		struct itimerval end_tmr;
//		struct itimerval last_tmr;
		
		unsigned int Ncalls;
//		unsigned int last_Ncalls;
//		unsigned int PERIOD_EVENTS;
		
		typedef struct{
			double tot_rate;            ///< Instantaneous rate due to all threads
			double tot_integrated_rate; ///< Total integrated rate for entire job due to all threads
			double thread_rate;         ///< Last instantaneous rate of this thread (updated every 2 seconds)
			double thread_delta_sec;    ///< Time to process the previous event (not this one!)
			double cpu;
			double mem_MB;
			unsigned int threadid;      ///< pthreadid of this thread
		}rate_t;
		
		rate_t rate;
		
		TTree *rate_tree;
};
