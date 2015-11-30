// $Id:$
//
//    File: Associated_test.cc
// Created: Thu May 21 10:54:02 EDT 2015
// Creator: davidl (on Linux gluon47 2.6.32-358.23.2.el6.x86_64)
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
using namespace std;
		 
#include <JANA/JApplication.h>
#include <JANA/JResourceManager.h>
using namespace jana;

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"

int NARG;
char **ARGV;

#include "JFactoryGenerator_TestGenerator.h"
#include "JEventProcessor_TestProcessor.h"

//
// This tests the associated object mechanism of JANA.
// For this series of tests, the following associations
// are created in the various TestClass factories.
//
//
//       ----------
//      /          \.
//      D  D------  \. 
//     /  / \     \  \.  
//     C  C  C---  \  \.  
//    /  /  / \  \  \  \.  
//    B  B  B  B  \  \  \.  
//   /  /  /  / \  \  \  \.  
//   A  A  A  A  A  A  A  A
//


//------------------
// main
//------------------
int main(int narg, char *argv[])
{
	cout<<endl;
	jout<<"----- starting AssociatedObject unit test ------"<<endl;

	// Record command line args for later use
	NARG = narg;
	ARGV = argv;

	int result = Catch::Main( narg, argv );

	return result;
}

//------------------
// TEST_CASE
//------------------
TEST_CASE("primary/associated objects", "Gets all primary and associated objects")
{
	// Create JApplication for use with first 3 tests
	JApplication *app = new JApplication(NARG, ARGV);
	JEventProcessor_TestProcessor *proc = new JEventProcessor_TestProcessor;
	
	app->AddFactoryGenerator(new JFactoryGenerator_TestGenerator);
	app->AddProcessor(proc);

	gPARMS->SetParameter("PLUGINS", "TestSpeed");
	gPARMS->SetParameter("EVENTS_TO_KEEP", 1);

	app->Run();
	
	map<string, map<string, vector< vector<const JObject*> > > > &all = proc->associated_all;
	map<string, map<string, vector< vector<const JObject*> > > > &direct = proc->associated_direct;


 	jout << "------------------ Test 1 (primary objects)-------------------" << endl;
 	REQUIRE( proc->objsA.size() == 8 );
	REQUIRE( proc->objsB.size() == 4 );
	REQUIRE( proc->objsC.size() == 3 );
	REQUIRE( proc->objsD.size() == 2 );

 	jout << "------------------ Test 2 (direct associations)-------------------" << endl;
 	REQUIRE( direct.size() == 4 );
	REQUIRE( direct["TestClassA"].size() == 4 );
	REQUIRE( direct["TestClassB"].size() == 4 );
	REQUIRE( direct["TestClassC"].size() == 4 );
	REQUIRE( direct["TestClassD"].size() == 4 );

	REQUIRE( direct["TestClassA"]["TestClassA"].size() == 8 );
	REQUIRE( direct["TestClassA"]["TestClassB"].size() == 8 );
	REQUIRE( direct["TestClassA"]["TestClassC"].size() == 8 );
	REQUIRE( direct["TestClassA"]["TestClassD"].size() == 8 );

	REQUIRE( direct["TestClassB"]["TestClassA"].size() == 4 );
	REQUIRE( direct["TestClassB"]["TestClassA"][0].size() == 1 );
	REQUIRE( direct["TestClassB"]["TestClassA"][3].size() == 2 );

	REQUIRE( direct["TestClassC"]["TestClassA"].size() == 3 );
	REQUIRE( direct["TestClassC"]["TestClassA"][0].size() == 0 );
	REQUIRE( direct["TestClassC"]["TestClassA"][2].size() == 1 );
	REQUIRE( direct["TestClassC"]["TestClassB"][0].size() == 1 );
	REQUIRE( direct["TestClassC"]["TestClassB"][2].size() == 2 );

	REQUIRE( direct["TestClassD"]["TestClassA"].size() == 2 );
	REQUIRE( direct["TestClassD"]["TestClassA"][0].size() == 1 );
	REQUIRE( direct["TestClassD"]["TestClassA"][1].size() == 1 );
	REQUIRE( direct["TestClassD"]["TestClassB"][0].size() == 0 );
	REQUIRE( direct["TestClassD"]["TestClassC"][0].size() == 1 );
	REQUIRE( direct["TestClassD"]["TestClassC"][1].size() == 2 );

 	jout << "------------------ Test 3 (ancestor associations)-------------------" << endl;
	REQUIRE( all["TestClassA"]["TestClassA"].size() == 8 );
	REQUIRE( all["TestClassA"]["TestClassB"].size() == 8 );
	REQUIRE( all["TestClassA"]["TestClassC"].size() == 8 );
	REQUIRE( all["TestClassA"]["TestClassD"].size() == 8 );

	REQUIRE( all["TestClassB"]["TestClassA"].size() == 4 );
	REQUIRE( all["TestClassB"]["TestClassA"][0].size() == 1 );
	REQUIRE( all["TestClassB"]["TestClassA"][3].size() == 2 );

	REQUIRE( all["TestClassC"]["TestClassA"].size() == 3 );
	REQUIRE( all["TestClassC"]["TestClassA"][0].size() == 1 );
	REQUIRE( all["TestClassC"]["TestClassA"][2].size() == 4 );
	REQUIRE( all["TestClassC"]["TestClassB"][0].size() == 1 );
	REQUIRE( all["TestClassC"]["TestClassB"][2].size() == 2 );

	REQUIRE( all["TestClassD"]["TestClassA"].size() == 2 );
	REQUIRE( all["TestClassD"]["TestClassA"][0].size() == 2 );
	REQUIRE( all["TestClassD"]["TestClassA"][1].size() == 6 );
	REQUIRE( all["TestClassD"]["TestClassB"][0].size() == 1 );
	REQUIRE( all["TestClassD"]["TestClassB"][1].size() == 3 );
	REQUIRE( all["TestClassD"]["TestClassC"][0].size() == 1 );
	REQUIRE( all["TestClassD"]["TestClassC"][1].size() == 2 );
 

	delete app;
}



