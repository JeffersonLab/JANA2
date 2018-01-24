// $Id$
//
//    File: jana_test_factory.cc
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "jana_test_factory.h"

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

