// $Id$
//
//    File: JEventSourceGenerator_MySource.h
// Created: Mon Dec 20 08:18:56 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#ifndef _JEventSourceGenerator_MySource_
#define _JEventSourceGenerator_MySource_

#include <JANA/jerror.h>
#include <JANA/JEventSourceGenerator.h>

#include "JEventSource_MySource.h"

class JEventSourceGenerator_MySource: public jana::JEventSourceGenerator{
	public:
		JEventSourceGenerator_MySource(){}
		virtual ~JEventSourceGenerator_MySource(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator_MySource";}
		
		const char* Description(void);
		double CheckOpenable(string source);
		jana::JEventSource* MakeJEventSource(string source);
};

#endif // _JEventSourceGenerator_MySource_

