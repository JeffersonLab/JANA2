// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;


class JEventProcessorTest:public JEventProcessor
{
	public:
		const char* className(void){return "JEventProcessorTest";}
		
		jerror_t init(void);				///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, uint64_t eventnumber);						///< Called every event.
		jerror_t erun(void){return NOERROR;};				///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);				///< Called after last event of last event source has been processed.

};
