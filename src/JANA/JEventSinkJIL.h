// Author: David Lawrence  June 25, 2004
//
//
// JEventSinkJIL.h
//
/// Example program for a Hall-D analyzer which uses DANA
///

#include "hd_serializers.h"

#include "JEventProcessor.h"
#include "JEventLoop.h"
#include "JEventSink.h"


class JEventSinkJIL:public JEventSink
{
	public:
		JEventSinkJIL(const char* filename){this->filename = filename;}
	
		jerror_t brun_sink(JEventLoop *loop, int runnumber);
		
		jerror_t init(void);										///< Called once at program start.
		jerror_t evnt(JEventLoop *eventLoop, int eventnumber);						///< Called every event.
		jerror_t erun(void){return NOERROR;}				///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);										///< Called after last event of last event source has been processed.

		JILStream *s;
		const char* filename;
	
	private:
		JEventSinkJIL(){} /// prevent use of default constructor
};
