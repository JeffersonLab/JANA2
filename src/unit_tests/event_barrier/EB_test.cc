// $Id:$
//
//    File: EB_test.cc
// Created: Tue Jun  9 11:29:29 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 Darwin Kernel Version 13.4.0)
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"

int NARG;
char **ARGV;

#include "JEventSourceGenerator_EBTest.h"
#include "JEventProcessor_EBTest.h"

//
// This tests the event barrier mechanism of JANA. This is
// a mechanism designed to handle things like EPICs events
// where a special event may set globals used by all
// threads. The event source sets this flag to signal
// JANA that all events before this should finish processing
// before it is processed and the special event should
// finish processing before normal processing is allowed to
// resume.



//------------------
// main
//------------------
int main(int narg, char *argv[])
{
	cout<<endl;
	jout<<"----- starting EventBarrier unit test ------"<<endl;

	// Record command line args for later use
	NARG = narg;
	ARGV = argv;

	int result = Catch::Main( narg, argv );

	return result;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("event barrier", "Checks event barrier mechanism")
{
	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	JEventProcessor_EBTest *proc = new JEventProcessor_EBTest;
	
	gPARMS->SetParameter("EVENTS_TO_KEEP", 1000);
	gPARMS->SetParameter("NTHREADS", "Ncores");
	app->AddEventSource("dummy");
	
	app->AddEventSourceGenerator(new JEventSourceGenerator_EBTest);
	app->AddProcessor(proc);

	app->Run();
	
 	jout << "------------------ Test 1 -------------------" << endl;
	REQUIRE( proc->Nerrors == 0 );
 

	delete app;
}



