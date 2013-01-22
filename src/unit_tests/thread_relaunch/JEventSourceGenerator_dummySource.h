// $Id$
//
//    File: JEventSourceGenerator_dummySource.h
// Created: Tue Jan 22 11:33:11 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _JEventSourceGenerator_dummySource_
#define _JEventSourceGenerator_dummySource_

#include <JANA/jerror.h>
#include <JANA/JEventSourceGenerator.h>

#include "JEventSource_dummySource.h"

class JEventSourceGenerator_dummySource: public jana::JEventSourceGenerator{
	public:
		JEventSourceGenerator_dummySource(){}
		virtual ~JEventSourceGenerator_dummySource(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator_dummySource";}
		
		const char* Description(void);
		double CheckOpenable(string source);
		jana::JEventSource* MakeJEventSource(string source);
};

#endif // _JEventSourceGenerator_dummySource_

