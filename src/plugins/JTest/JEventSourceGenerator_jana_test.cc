// $Id$
//
//    File: JEventSourceGenerator_jana_test.cc
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include <string>
using std::string;

#include "JEventSourceGenerator_jana_test.h"


//---------------------------------
// Description
//---------------------------------
const char* JEventSourceGenerator_jana_test::Description(void)
{
	return "jana_test";
}

//---------------------------------
// CheckOpenable
//---------------------------------
double JEventSourceGenerator_jana_test::CheckOpenable(string source)
{
	// This should return a value between 0 and 1 inclusive
	// with 1 indicating it definitely can read events from
	// the specified source and 0 meaning it definitely can't.
	// Typically, this will just check the file suffix.
	
	return 0.5; 
}

//---------------------------------
// MakeJEventSource
//---------------------------------
JEventSource* JEventSourceGenerator_jana_test::MakeJEventSource(string source)
{
	return new JEventSource_jana_test(source.c_str());
}

