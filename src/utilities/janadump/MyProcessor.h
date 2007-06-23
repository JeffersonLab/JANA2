// $Id: MyProcessor.h 1899 2006-07-13 16:29:56Z davidl $
// Author: David Lawrence  June 25, 2004
//
//
// MyProcessor.h
//
/// janadump print event info to screen
///

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
#include <JANA/JFactory.h>

extern int PAUSE_BETWEEN_EVENTS;
extern int SKIP_BORING_EVENTS;
extern int PRINT_ALL;

extern vector<string> toprint;

class MyProcessor:public JEventProcessor
{
	public:
		jerror_t init(void){return NOERROR;};				///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, int eventnumber);						///< Called every event.
		jerror_t erun(void){return NOERROR;};				///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void){return NOERROR;};				///< Called after last event of last event source has been processed.

		typedef struct{
			string dataClassName;
			string tag;
		}factory_info_t;
		vector<factory_info_t> fac_info;
};
