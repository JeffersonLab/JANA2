// Author: David Lawrence  August 8, 2007
//
//

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
#include "JEventProcessorTest.h"
#include "JTest.h"


//------------------------------------------------------------------
// init 
//------------------------------------------------------------------
jerror_t JEventProcessorTest::init(void)
{
	// Nothing to do here.

	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorTest::brun(JEventLoop *loop, int runnumber)
{
	
	cout<<"brun(...) method called of JEventProcessorTest"<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorTest::evnt(JEventLoop *loop, int eventnumber)
{
	vector<const JTest*> jtests;
	loop->Get(jtests);

	return NOERROR;
}

//------------------------------------------------------------------
// fini   -Close output file here
//------------------------------------------------------------------
jerror_t JEventProcessorTest::fini(void)
{
	return NOERROR;
}

