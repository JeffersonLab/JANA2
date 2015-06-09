// $Id$
//
//    File: JEventSourceGenerator_EBTest.h
// Created: Tue Jun  9 11:28:14 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventSourceGenerator_EBTest_
#define _JEventSourceGenerator_EBTest_

#include <JANA/jerror.h>
#include <JANA/JEventSourceGenerator.h>

#include "JEventSource_EBTest.h"

class JEventSourceGenerator_EBTest: public jana::JEventSourceGenerator{
	public:
		JEventSourceGenerator_EBTest(){}
		virtual ~JEventSourceGenerator_EBTest(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator_EBTest";}
		
		const char* Description(void){return "EBTest source";}
		double CheckOpenable(string source){return 1.0;}
		jana::JEventSource* MakeJEventSource(string source){return new JEventSource_EBTest(source.c_str());}
};

#endif // _JEventSourceGenerator_EBTest_

