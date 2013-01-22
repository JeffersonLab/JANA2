// $Id$
//
//    File: JEventSourceGenerator_dummySource.cc
// Created: Tue Jan 22 11:33:11 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#include <string>
using std::string;

#include "JEventSourceGenerator_dummySource.h"
using namespace jana;


//---------------------------------
// Description
//---------------------------------
const char* JEventSourceGenerator_dummySource::Description(void)
{
	return "dummySource";
}

//---------------------------------
// CheckOpenable
//---------------------------------
double JEventSourceGenerator_dummySource::CheckOpenable(string source)
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
JEventSource* JEventSourceGenerator_dummySource::MakeJEventSource(string source)
{
	return new JEventSource_dummySource(source.c_str());
}

