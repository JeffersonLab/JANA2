// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>

#include <map>
using std::map;

class JEventProcessorJANADOT:public JEventProcessor
{
	public:
		jerror_t init(void);							///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, int eventnumber);	///< Called every event.
		jerror_t erun(void){return NOERROR;};	///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);							///< Called after last event of last event source has been processed.

		class CallLink{
			public:
				string caller_name;
				string caller_tag;
				string callee_name;
				string callee_tag;
				
				bool operator<(const CallLink &link) const {
					if(this->caller_name!=link.caller_name)return this->caller_name < link.caller_name;
					if(this->callee_name!=link.callee_name)return this->callee_name < link.callee_name;
					if(this->caller_tag!=link.caller_tag)return this->caller_tag < link.caller_tag;
					return this->callee_tag < link.callee_tag;
				}
		};
		
		class CallStats{
			public:
				CallStats(void){
					from_cache_ticks = 0;
					from_source_ticks = 0;
					from_factory_ticks = 0;
					data_not_available_ticks = 0;
					Nfrom_cache = 0;
					Nfrom_source = 0;
					Nfrom_factory = 0;
					Ndata_not_available = 0;
				}
				clock_t from_cache_ticks;
				clock_t from_source_ticks;
				clock_t from_factory_ticks;
				clock_t data_not_available_ticks;
				unsigned int Nfrom_cache;
				unsigned int Nfrom_source;
				unsigned int Nfrom_factory;
				unsigned int Ndata_not_available;
		};

		map<CallLink, CallStats> call_links;
		pthread_mutex_t mutex;
		bool force_all_factories_active;
};
