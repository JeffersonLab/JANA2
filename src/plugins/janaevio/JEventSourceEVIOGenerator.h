// $Id$
//
//    File: JEventSourceEVIOGenerator.h
// Created: Fri Jul 14 12:44:59 EDT 2006
// Creator: davidl (on Darwin Harriet.local 8.6.0 powerpc)
//

#ifndef _JEventSourceEVIOGenerator_
#define _JEventSourceEVIOGenerator_

#include <JANA/JEventSourceGenerator.h>
using namespace jana;

class JEventSourceEVIOGenerator:public JEventSourceGenerator{
	public:
		JEventSourceEVIOGenerator(){}
		~JEventSourceEVIOGenerator(){}
		const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceEVIOGenerator";}
		
		const char* Description(void);
		double CheckOpenable(string source);
		JEventSource* MakeJEventSource(string source);
		
};

#endif // _JEventSourceEVIOGenerator_

