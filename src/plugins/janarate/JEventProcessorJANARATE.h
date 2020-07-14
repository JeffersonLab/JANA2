// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JEventProcessor.h>

#include <map>
using std::map;

class TTree;

class JEventProcessorJANARATE:public JEventProcessor
{
	public:

	JEventProcessorJANARATE(JApplication* app): JEventProcessor(app) {}

	virtual void Init(void);
	virtual void Process(const std::shared_ptr<const JEvent>& aEvent);
	virtual void Finish(void);

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
