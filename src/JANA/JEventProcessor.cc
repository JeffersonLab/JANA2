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
	pthread_mutex_init(&state_mutex, NULL);
	app = NULL;
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
jerror_t JEventProcessor::brun(JEventLoop *loop, int runnumber)
{
	return NOERROR;
}

//----------------
// evnt
//----------------
jerror_t JEventProcessor::evnt(JEventLoop *eventLoop, int eventnumber)
{
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
