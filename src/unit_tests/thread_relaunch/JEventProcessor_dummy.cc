// $Id$
//
//    File: JEventProcessor_dummy.cc
// Created: Tue Jan 22 09:43:15 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#include "JEventProcessor_dummy.h"
using namespace jana;

#include "stalling.h"


//------------------
// JEventProcessor_dummy (Constructor)
//------------------
JEventProcessor_dummy::JEventProcessor_dummy()
{

}

//------------------
// ~JEventProcessor_dummy (Destructor)
//------------------
JEventProcessor_dummy::~JEventProcessor_dummy()
{

}

//------------------
// init
//------------------
jerror_t JEventProcessor_dummy::init(void)
{
	// This is called once at program startup
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_dummy::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	// This is called whenever the run number changes
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_dummy::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	vector<const stalling*> stallings;
	loop->Get(stallings);

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_dummy::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_dummy::fini(void)
{
	// Called before program exit after event processing is finished.
	return NOERROR;
}

