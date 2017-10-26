// $Id$
//
//    File: jana_test_factory.cc
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//


#include <iostream>
#include <iomanip>
using namespace std;

#include "jana_test_factory.h"
#include "JEventSourceGenerator_jana_test.h"
#include "JFactoryGenerator_jana_test.h"
#include "JEventProcessor_jana_test.h"

// Routine used to create our JEventProcessor
#include <JANA/JApplication.h>
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddJEventSourceGenerator(new JEventSourceGenerator_jana_test());
	app->AddJFactoryGenerator(new JFactoryGenerator_jana_test());
	app->AddJEventProcessor(new JEventProcessor_jana_test());
}
} // "C"

//------------------
// init
//------------------
void jana_test_factory::init(void)
{

}

//------------------
// brun
//------------------
void jana_test_factory::brun(JEvent *jevent, int32_t runnumber)
{

}

//------------------
// evnt
//------------------
void jana_test_factory::evnt(JEvent *jevent, uint64_t eventnumber)
{

	// Code to generate factory data goes here. Add it like:
	//
	// jana_test *myjana_test = new jana_test;
	// myjana_test->x = x;
	// myjana_test->y = y;
	// ...
	// _data.push_back(myjana_test);
	//
	// Note that the objects you create here will be deleted later
	// by the system and the _data vector will be cleared automatically.

}

//------------------
// erun
//------------------
void jana_test_factory::erun(void)
{

}

//------------------
// fini
//------------------
void jana_test_factory::fini(void)
{

}

