// $Id$
//
//    File: JFactoryGenerator_TestGenerator.h
// Created: Thu May 21 15:28:03 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _JFactoryGenerator_TestGenerator_
#define _JFactoryGenerator_TestGenerator_

#include <JANA/jerror.h>
#include <JANA/JFactoryGenerator.h>

#include "TestClassA_factory.h"
#include "TestClassB_factory.h"
#include "TestClassC_factory.h"
#include "TestClassD_factory.h"


class JFactoryGenerator_TestGenerator: public jana::JFactoryGenerator{
	public:
		JFactoryGenerator_TestGenerator(){}
		virtual ~JFactoryGenerator_TestGenerator(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator_TestGenerator";}
		
		jerror_t GenerateFactories(jana::JEventLoop *loop){
			loop->AddFactory(new TestClassA_factory());
			loop->AddFactory(new TestClassB_factory());
			loop->AddFactory(new TestClassC_factory());
			loop->AddFactory(new TestClassD_factory());
			return NOERROR;
		}

};

#endif // _JFactoryGenerator_TestGenerator_

