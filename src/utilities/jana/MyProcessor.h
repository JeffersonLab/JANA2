// Author: David Lawrence  June 25, 2004
//
//
// MyProcessor.h
//
/// Example program for a Hall-D analyzer which uses DANA
///

#include <map>
using std::map;

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

extern map<string, string> autoactivate;
extern bool ACTIVATE_ALL;

class MyProcessor:public JEventProcessor
{
	public:
		jerror_t init(void);				///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, uint64_t eventnumber);						///< Called every event.
		jerror_t erun(void){return NOERROR;};				///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);				///< Called after last event of last event source has been processed.

		map<string, string> autofactories;

};
