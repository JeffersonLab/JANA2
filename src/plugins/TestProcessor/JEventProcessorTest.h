// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>


class JEventProcessorTest:public JEventProcessor
{
	public:
		jerror_t init(void);				///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, int eventnumber);						///< Called every event.
		jerror_t erun(void){return NOERROR;};				///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);				///< Called after last event of last event source has been processed.

};
