// $Id$
//
//    File: JResourceManager_test.cc
// Created: Thu Oct 25 09:13:59 EDT 2012
// Creator: davidl (on Linux ifarm1101 2.6.18-274.3.1.el5 x86_64)
//

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
#include <JANA/JResourceManager.h>
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "JFactoryGenerator_stalling.h"
#include "JEventProcessor_dummy.h"
#include "JEventSourceGenerator_dummySource.h"

int NARG;
char **ARGV;

unsigned int TestResourceManager_Option1(void);
unsigned int TestResourceManager_Option2(void);

//------------------
// main
//------------------
int main(int narg, char *argv[])
{
	cout<<endl;
	jout<<"----- starting JResourceManager unit test ------"<<endl;

	// Record command line args for later use
	NARG = narg;
	ARGV = argv;

	int result = Catch::Main( narg, argv );

	return result;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("thread_relaunch: single thread, no-auto", "Gets a remote resource using JResourceManager through JApplication")
{
	/// Single thread, no auto thread launching

	jout << "------------------ Test 1 -------------------" << endl;

	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	
	app->AddFactoryGenerator(new JFactoryGenerator_stalling);
	app->AddProcessor(new JEventProcessor_dummy);
	app->AddEventSourceGenerator(new JEventSourceGenerator_dummySource);
	app->AddEventSource("fake_event_generator");
	
	uint32_t EVENTS_TO_KEEP = 1370;
	gPARMS->SetParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP);
	uint32_t THREAD_TIMEOUT = 1;
	gPARMS->SetParameter("THREAD_TIMEOUT", THREAD_TIMEOUT);
	uint32_t MAX_RELAUNCH_THREADS = 0;
	gPARMS->SetParameter("JANA:MAX_RELAUNCH_THREADS", MAX_RELAUNCH_THREADS);
	
	app->Run();
	
	jout<<" Nevents=" << app->GetNEvents() << endl;
	jout<<" Nlost_events=" << app->GetNLostEvents() << endl;
	REQUIRE( app->GetNEvents() ==  137);
	REQUIRE( app->GetNLostEvents() ==  1);

	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("thread_relaunch: single thread, with-auto", "Gets a remote resource using JResourceManager through JApplication")
{
	/// Single thread, no auto thread launching

	jout << "------------------ Test 2 -------------------" << endl;

	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	
	app->AddFactoryGenerator(new JFactoryGenerator_stalling);
	app->AddProcessor(new JEventProcessor_dummy);
	app->AddEventSourceGenerator(new JEventSourceGenerator_dummySource);
	app->AddEventSource("fake_event_generator");
	
	uint32_t EVENTS_TO_KEEP = 1370;
	gPARMS->SetParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP);
	uint32_t THREAD_TIMEOUT = 1;
	gPARMS->SetParameter("THREAD_TIMEOUT", THREAD_TIMEOUT);
	uint32_t MAX_RELAUNCH_THREADS = 10;
	gPARMS->SetParameter("JANA:MAX_RELAUNCH_THREADS", MAX_RELAUNCH_THREADS);
	
	app->Run();
	
	jout<<" Nevents=" << app->GetNEvents() << endl;
	jout<<" Nlost_events=" << app->GetNLostEvents() << endl;
	REQUIRE( app->GetNEvents() ==  EVENTS_TO_KEEP);
	REQUIRE( app->GetNLostEvents() ==  EVENTS_TO_KEEP/137);

	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("thread_relaunch: multi-thread, no-auto", "Gets a remote resource using JResourceManager through JApplication")
{
	/// Single thread, no auto thread launching

	jout << "------------------ Test 3 -------------------" << endl;

	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	
	app->AddFactoryGenerator(new JFactoryGenerator_stalling);
	app->AddProcessor(new JEventProcessor_dummy);
	app->AddEventSourceGenerator(new JEventSourceGenerator_dummySource);
	app->AddEventSource("fake_event_generator");
	
	uint32_t EVENTS_TO_KEEP = 1370;
	gPARMS->SetParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP);
	uint32_t THREAD_TIMEOUT = 1;
	gPARMS->SetParameter("THREAD_TIMEOUT", THREAD_TIMEOUT);
	uint32_t MAX_RELAUNCH_THREADS = 0;
	gPARMS->SetParameter("JANA:MAX_RELAUNCH_THREADS", MAX_RELAUNCH_THREADS);
	
	int Nthreads = 4;
	app->Run(NULL,Nthreads);
	
	jout<<" Nevents=" << app->GetNEvents() << endl;
	jout<<" Nlost_events=" << app->GetNLostEvents() << endl;
	REQUIRE( app->GetNEvents() ==  Nthreads*137);
	REQUIRE( app->GetNLostEvents() ==  Nthreads);

	delete app;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("thread_relaunch: multi-thread, with-auto", "Gets a remote resource using JResourceManager through JApplication")
{
	/// Single thread, no auto thread launching

	jout << "------------------ Test 4 -------------------" << endl;

	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	
	app->AddFactoryGenerator(new JFactoryGenerator_stalling);
	app->AddProcessor(new JEventProcessor_dummy);
	app->AddEventSourceGenerator(new JEventSourceGenerator_dummySource);
	app->AddEventSource("fake_event_generator");
	
	uint32_t EVENTS_TO_KEEP = 1370;
	gPARMS->SetParameter("EVENTS_TO_KEEP", EVENTS_TO_KEEP);
	uint32_t THREAD_TIMEOUT = 1;
	gPARMS->SetParameter("THREAD_TIMEOUT", THREAD_TIMEOUT);
	uint32_t MAX_RELAUNCH_THREADS = 10;
	gPARMS->SetParameter("JANA:MAX_RELAUNCH_THREADS", MAX_RELAUNCH_THREADS);
	
	int Nthreads = 4;
	app->Run(NULL,Nthreads);
	
	jout<<" Nevents=" << app->GetNEvents() << endl;
	jout<<" Nlost_events=" << app->GetNLostEvents() << endl;
	REQUIRE( app->GetNEvents() ==  EVENTS_TO_KEEP);
	REQUIRE( app->GetNLostEvents() ==  EVENTS_TO_KEEP/137);

	delete app;
}



