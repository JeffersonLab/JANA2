// $Id$
//
//    File: JFactoryGenerator_JPMUCounts.h
// Created: Sat Jul 17 06:03:14 EDT 2010
// Creator: davidl (on Linux ifarmltest 2.6.34 x86_64)
//

#ifndef _JFactoryGenerator_JPMUCounts_
#define _JFactoryGenerator_JPMUCounts_

#include <JANA/jerror.h>
#include <JANA/JFactoryGenerator.h>

#include "JPMUCounts_factory.h"

class JFactoryGenerator_JPMUCounts: public jana::JFactoryGenerator{
	public:
		JFactoryGenerator_JPMUCounts(){}
		virtual ~JFactoryGenerator_JPMUCounts(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator_JPMUCounts";}
		
		jerror_t GenerateFactories(jana::JEventLoop *loop){
			loop->AddFactory(new JPMUCounts_factory());
			return NOERROR;
		}

};

#endif // _JFactoryGenerator_JPMUCounts_

