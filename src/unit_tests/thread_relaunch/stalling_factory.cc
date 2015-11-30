// $Id$
//
//    File: stalling_factory.cc
// Created: Tue Jan 22 09:41:30 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#include <unistd.h>

#include <iostream>
#include <iomanip>
using namespace std;

#include "stalling_factory.h"
using namespace jana;

//------------------
// init
//------------------
jerror_t stalling_factory::init(void)
{
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t stalling_factory::brun(jana::JEventLoop *eventLoop, int32_t runnumber)
{
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t stalling_factory::evnt(JEventLoop *loop, uint64_t eventnumber)
{

	if( (eventnumber%137) == 0 ) sleep(60);


	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t stalling_factory::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t stalling_factory::fini(void)
{
	return NOERROR;
}

