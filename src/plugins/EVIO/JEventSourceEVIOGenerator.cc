// $Id$
//
//    File: JEventSourceEVIOGenerator.cc
// Created: Fri Jul 14 12:44:59 EDT 2006
// Creator: davidl (on Darwin Harriet.local 8.6.0 powerpc)
//


#include <string>
using std::string;

#include "JEventSourceEVIOGenerator.h"
#include "JEventSourceEVIO.h"

//---------------------------------
// Description
//---------------------------------
const char* JEventSourceEVIOGenerator::Description(void)
{
	return "EVIO";
}

//---------------------------------
// CheckOpenable
//---------------------------------
double JEventSourceEVIOGenerator::CheckOpenable(string source)
{
	/// This needs to be fixed. Right now, it just looks for a .evio suffix
	return source.find(".evio",0)==string::npos ? 0.0:1.0;
}

//---------------------------------
// MakeJEventSource
//---------------------------------
JEventSource* JEventSourceEVIOGenerator::MakeJEventSource(string source)
{
	return new JEventSourceEVIO(source.c_str());
}
		

