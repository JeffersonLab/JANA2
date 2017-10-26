// $Id$
//
//    File: JEventProcessor_jana_test.cc
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "JEventProcessor_jana_test.h"


//------------------
// JEventProcessor_jana_test (Constructor)
//------------------
JEventProcessor_jana_test::JEventProcessor_jana_test()
{

}

//------------------
// ~JEventProcessor_jana_test (Destructor)
//------------------
JEventProcessor_jana_test::~JEventProcessor_jana_test()
{

}

//------------------
// init
//------------------
void JEventProcessor_jana_test::init(void)
{
	// This is called once at program startup
}

//------------------
// brun
//------------------
void JEventProcessor_jana_test::brun(JEvent *jevent, int32_t runnumber)
{
	// This is called whenever the run number changes
}

//------------------
// evnt
//------------------
void JEventProcessor_jana_test::evnt(JEvent *jevent, uint64_t eventnumber)
{
	// This is called for every event. Use of common resources like writing
	// to a file or filling a histogram should be mutex protected. Using
	// loop->Get(...) to get reconstructed objects (and thereby activating the
	// reconstruction algorithm) should be done outside of any mutex lock
	// since multiple threads may call this method at the same time.

}

//------------------
// erun
//------------------
void JEventProcessor_jana_test::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.
}

//------------------
// fini
//------------------
void JEventProcessor_jana_test::fini(void)
{
	// Called before program exit after event processing is finished.
}

