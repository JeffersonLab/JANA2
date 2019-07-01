// $Id$
//
//    File: JFactoryGenerator_jana_test.h
// Created: Mon Oct 23 22:39:33 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JFactoryGenerator_jana_test_
#define _JFactoryGenerator_jana_test_

#include <vector>

#include <JANA/JFactoryGenerator.h>
#include "JResourcePool.h"

#include "jana_test_factory.h"

class JFactoryGenerator_jana_test : public JFactoryGenerator
{
	public:
		JFactoryGenerator_jana_test(){}
		virtual ~JFactoryGenerator_jana_test(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator_jana_test";}
		
		void GenerateFactories(JFactorySet *factory_set){
			factory_set->Add( new jana_test_factory() );
		}
};

#endif // _JFactoryGenerator_jana_test_

