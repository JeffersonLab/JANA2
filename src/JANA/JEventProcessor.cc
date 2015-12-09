// $Id: JEventProcessor.cc 1039 2005-06-14 20:21:02Z davidl $
//
//    File: JEventProcessor.cc
// Created: Wed Jun  8 12:31:12 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include "JEventProcessor.h"
using namespace jana;

//---------------------------------
// JEventProcessor    (Constructor)
//---------------------------------
JEventProcessor::JEventProcessor(void)
{

	init_called = 0;
	brun_called = 0;
	evnt_called = 0;
	erun_called = 0;
	fini_called = 0;
	brun_runnumber = -1; // ensure brun is called
	brun_eventnumber = 0;
	pthread_mutex_init(&state_mutex, NULL);
	app = NULL;
	delete_me = false; // n.b. this ALWAYS gets overwritten when JApplication::AddProcessor is called!!
}

//---------------------------------
// ~JEventProcessor    (Destructor)
//---------------------------------
JEventProcessor::~JEventProcessor()
{

}

//----------------
// init
//----------------
jerror_t JEventProcessor::init(void)
{
	return NOERROR;
}

//----------------
// brun
//----------------
jerror_t JEventProcessor::brun(JEventLoop *loop, int32_t runnumber)
{
	return NOERROR;
}

////----------------
//// evnt  (DEPRECATED)
////----------------
//jerror_t JEventProcessor::evnt(JEventLoop *loop, int eventnumber)
//{
//	return NOERROR;
//}

//----------------
// evnt
//----------------
jerror_t JEventProcessor::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	// redirect to "int" version for now so old code still works
//	return evnt(loop, (int)eventnumber); // this caused hangs in sim-recon in jana 0.7.4
	return NOERROR;
}

//----------------
// erun
//----------------
jerror_t JEventProcessor::erun(void)
{
	return NOERROR;
}

//----------------
// fini
//----------------
jerror_t JEventProcessor::fini(void)
{
	return NOERROR;
}
