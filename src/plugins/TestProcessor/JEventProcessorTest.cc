// Author: David Lawrence  August 8, 2007
//
//

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
#include "JEventProcessorTest.h"


// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new JEventProcessorTest());
}
} // "C"

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
jerror_t JEventProcessorTest::brun(JEventLoop *eventLoop, int runnumber)
{
	
	cout<<"brun(...) method called of JEventProcessorTest"<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorTest::evnt(JEventLoop *eventLoop, int eventnumber)
{
	//cout<<"evnt(...) method called of JEventProcessorTest"<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// fini   -Close output file here
//------------------------------------------------------------------
jerror_t JEventProcessorTest::fini(void)
{
	return NOERROR;
}

