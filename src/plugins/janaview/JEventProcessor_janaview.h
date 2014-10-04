// $Id$
//
//    File: JEventProcessor_janaview.h
// Created: Fri Oct  3 08:14:14 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventProcessor_janaview_
#define _JEventProcessor_janaview_

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include "jv_mainframe.h"

#include <JVFactoryInfo.h>

class JEventProcessor_janaview:public jana::JEventProcessor{
	public:
		JEventProcessor_janaview();
		~JEventProcessor_janaview();
		const char* className(void){return "JEventProcessor_janaview";}
		
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		JEventLoop *loop;
		int eventnumber;

		void Lock(void){pthread_mutex_lock(&mutex);}
		void Unlock(void){pthread_mutex_unlock(&mutex);}
		void NextEvent(void);
		void GetObjectTypes(vector<JVFactoryInfo> &facinfo);

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

extern JEventProcessor_janaview *JEP;

#endif // _JEventProcessor_janaview_

