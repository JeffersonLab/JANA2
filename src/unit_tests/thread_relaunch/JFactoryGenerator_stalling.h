// $Id$
//
//    File: JFactoryGenerator_stalling.h
// Created: Tue Jan 22 09:39:40 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _JFactoryGenerator_stalling_
#define _JFactoryGenerator_stalling_

#include <JANA/jerror.h>
#include <JANA/JFactoryGenerator.h>

#include "stalling_factory.h"

class JFactoryGenerator_stalling: public jana::JFactoryGenerator{
	public:
		JFactoryGenerator_stalling(){}
		virtual ~JFactoryGenerator_stalling(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator_stalling";}
		
		jerror_t GenerateFactories(jana::JEventLoop *loop){
			loop->AddFactory(new stalling_factory());
			return NOERROR;
		}

};

#endif // _JFactoryGenerator_stalling_

