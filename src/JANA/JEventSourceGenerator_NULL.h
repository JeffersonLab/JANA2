// $Id$
//
//    File: JEventSourceGenerator_NULL.h
// Created: Thu Jun  4 15:52:36 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventSourceGenerator_NULL_
#define _JEventSourceGenerator_NULL_

#include <JANA/jerror.h>
#include <JANA/JEventSourceGenerator.h>

#include "JEventSource_NULL.h"

class JEventSourceGenerator_NULL: public jana::JEventSourceGenerator{
	public:
		JEventSourceGenerator_NULL(){}
		virtual ~JEventSourceGenerator_NULL(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator_NULL";}
		
		const char* Description(void){ return "NULL event source"; }
		double CheckOpenable(string source){ return 0.0; }
		jana::JEventSource* MakeJEventSource(string source){ return new JEventSource_NULL(source.c_str()); }
};

#endif // _JEventSourceGenerator_NULL_

