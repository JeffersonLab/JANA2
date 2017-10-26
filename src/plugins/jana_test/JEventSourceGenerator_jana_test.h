// $Id$
//
//    File: JEventSourceGenerator_jana_test.h
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventSourceGenerator_jana_test_
#define _JEventSourceGenerator_jana_test_

#include <JANA/JEventSourceGenerator.h>

#include "JEventSource_jana_test.h"

class JEventSourceGenerator_jana_test: public JEventSourceGenerator{
	public:
		JEventSourceGenerator_jana_test(){}
		virtual ~JEventSourceGenerator_jana_test(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator_jana_test";}
		
		const char* Description(void);
		double CheckOpenable(string source);
		JEventSource* MakeJEventSource(string source);
};

#endif // _JEventSourceGenerator_jana_test_

