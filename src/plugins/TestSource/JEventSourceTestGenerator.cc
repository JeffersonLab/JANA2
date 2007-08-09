// $Id$
//
//    File: JEventSourceTestGenerator.cc
// Created: Fri Jul 14 12:44:59 EDT 2006
// Creator: davidl (on Darwin Harriet.local 8.6.0 powerpc)
//


#include <string>
using std::string;

#include "JEventSourceTestGenerator.h"
#include "JEventSourceTest.h"

//---------------------------------
// Description
//---------------------------------
const char* JEventSourceTestGenerator::Description(void)
{
	return "Test";
}

//---------------------------------
// CheckOpenable
//---------------------------------
double JEventSourceTestGenerator::CheckOpenable(string source)
{
	/// This always returns 100% probability of 
	/// opening the source since it doesn't read from
	/// anything and should only be used for testing.
	return 1.0;
}

//---------------------------------
// MakeJEventSource
//---------------------------------
JEventSource* JEventSourceTestGenerator::MakeJEventSource(string source)
{
	return new JEventSourceTest(source.c_str());
}
		

