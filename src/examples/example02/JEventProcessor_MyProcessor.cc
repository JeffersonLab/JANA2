// $Id$
//
//    File: JEventProcessor_MyProcessor.cc
// Created: Mon Dec 20 09:36:38 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//


#include <iostream>
using namespace std;

#include "JEventProcessor_MyProcessor.h"
using namespace jana;

#include "RawHit.h"

//------------------
// JEventProcessor_MyProcessor (Constructor)
//------------------
JEventProcessor_MyProcessor::JEventProcessor_MyProcessor()
{

}

//------------------
// ~JEventProcessor_MyProcessor (Destructor)
//------------------
JEventProcessor_MyProcessor::~JEventProcessor_MyProcessor()
{

}

//------------------
// init
//------------------
jerror_t JEventProcessor_MyProcessor::init(void)
{
	// This is called once at program startup
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_MyProcessor::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	// This is called whenever the run number changes
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_MyProcessor::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	// This is called for every event. Use of common resources like writing
	// to a file or filling a histogram should be mutex protected. Using
	// loop->Get(...) to get reconstructed objects (and thereby activating the
	// reconstruction algorithm) should be done outside of any mutex lock
	// since multiple threads may call this method at the same time.

	// JANA idenitifies the appropriate algorithm by looking at the data type
	// of the container it is being requested to fill.
	vector<const RawHit*> hits;
	loop->Get(hits);
	
	// For this example, we simply print the number of RawHit objects found.
	cout<<"event "<<eventnumber<<": "<<hits.size()<<" RawHit objects found"<<endl;

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_MyProcessor::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_MyProcessor::fini(void)
{
	// Called before program exit after event processing is finished.
	return NOERROR;
}

