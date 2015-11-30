// $Id$
//
//    File: jana_user_references_test.cc
// Created: Tue Nov 26 10:47:47 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 13.0.0 Darwin Kernel Version 13.0.0 x86_64)
//

//=============================================================================
// This unit test checks the user reference system by creating multiple
// threads and having each create both a float and an int user reference.
// The references are used to keep track of the number of events each thread
// processes. Checks are done that the total number of events is correct
// and that the number of int and float references created is also correct.
//=============================================================================

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
#include "JEventProcessor_UserReference.h"
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"

int NARG;
char **ARGV;


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
TEST_CASE("user_reference", "Set and use a user reference")
{
	// Create JApplication
	JApplication *app = new JApplication(NARG, ARGV);

	// Attach TestSpeed plugin to provide events
	app->AddPlugin("TestSpeed");

	// Set Limit on number of events to process
	int events_to_keep = 100;
	gPARMS->SetDefaultParameter("EVENTS_TO_KEEP", events_to_keep);

	// Create event processor that actually uses the user reference
	JEventProcessor_UserReference *proc = new JEventProcessor_UserReference;

	// Run with 4 threads
	int Nthreads = 4;
	app->Run(proc, Nthreads);

	REQUIRE( proc->GetTotalI() == events_to_keep );
	REQUIRE( proc->GetTotalF() == (float)events_to_keep );
	REQUIRE( proc->GetNumCountersI() == Nthreads );
	REQUIRE( proc->GetNumCountersF() == Nthreads );

	delete app;
}


