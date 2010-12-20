// $Id$
//
//    File: JEventSourceGenerator_MySource.cc
// Created: Mon Dec 20 08:18:56 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#include <string>
using std::string;

#include "JEventSourceGenerator_MySource.h"
using namespace jana;


//---------------------------------
// Description
//---------------------------------
const char* JEventSourceGenerator_MySource::Description(void)
{
	return "MySource";
}

//---------------------------------
// CheckOpenable
//---------------------------------
double JEventSourceGenerator_MySource::CheckOpenable(string source)
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
JEventSource* JEventSourceGenerator_MySource::MakeJEventSource(string source)
{
	return new JEventSource_MySource(source.c_str());
}

