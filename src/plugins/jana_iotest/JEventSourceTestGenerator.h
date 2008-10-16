// $Id$
//
//    File: JEventSourceTestGenerator.h
// Created: Fri Jul 14 12:44:59 EDT 2006
// Creator: davidl (on Darwin Harriet.local 8.6.0 powerpc)
//

#ifndef _JEventSourceTestGenerator_
#define _JEventSourceTestGenerator_

#include "JANA/JEventSourceGenerator.h"
using namespace jana;

class JEventSourceTestGenerator:public JEventSourceGenerator{
	public:
		JEventSourceTestGenerator(){}
		~JEventSourceTestGenerator(){}
		const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceTestGenerator";}
		
		const char* Description(void);
		double CheckOpenable(string source);
		JEventSource* MakeJEventSource(string source);
		
};

#endif // _JEventSourceTestGenerator_

