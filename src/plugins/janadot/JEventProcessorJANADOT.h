// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

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

		enum node_type{
			kDefault,
			kProcessor,
			kFactory,
			kSource
		};

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
					from_cache_ms = 0;
					from_source_ms = 0;
					from_factory_ms = 0;
					data_not_available_ms = 0;
					Nfrom_cache = 0;
					Nfrom_source = 0;
					Nfrom_factory = 0;
					Ndata_not_available = 0;
				}
				double from_cache_ms;
				double from_source_ms;
				double from_factory_ms;
				double data_not_available_ms;
				unsigned int Nfrom_cache;
				unsigned int Nfrom_source;
				unsigned int Nfrom_factory;
				unsigned int Ndata_not_available;
		};
		
		class FactoryCallStats{
			public:
				FactoryCallStats(void){
					type = kDefault;
					time_waited_on = 0.0;
					time_waiting = 0.0;
					Nfrom_factory = 0;
					Nfrom_source = 0;
				}
				node_type type;
				double time_waited_on;	// time other factories spent waiting on this factory
				double time_waiting;		// time this factory spent waiting on other factories
				unsigned int Nfrom_factory;
				unsigned int Nfrom_source;
		};

		map<CallLink, CallStats> call_links;
		map<string, FactoryCallStats> factory_stats;
		pthread_mutex_t mutex;
		bool force_all_factories_active;
		
		string MakeTimeString(double time_in_ms);
		string MakeNametag(const string &name, const string &tag);
};
