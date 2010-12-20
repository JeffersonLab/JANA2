// $Id$
//
//    File: JFactoryGenerator_RawHit.h
// Created: Mon Dec 20 09:03:35 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#ifndef _JFactoryGenerator_RawHit_
#define _JFactoryGenerator_RawHit_

#include <JANA/jerror.h>
#include <JANA/JFactoryGenerator.h>

#include "RawHit.h"

// The following line is a shortcut that allows us to create
// a "factory" class for the RawHit objects without the overhead
// of creating RawHit_factory.cc and RawHit_factory.h files.
// This shortcut is useful mainly for objects supplied by the
// event source (e.g. read from file). This is because
// the algorithm object also acts as a container for the data
// objects, but for objects read from a file, no actual algorithm
// is needed. For an example with a fully implemented algorithm,
// see example03.
typedef JFactory<RawHit> RawHit_factory;

class JFactoryGenerator_RawHit: public jana::JFactoryGenerator{
	public:
		JFactoryGenerator_RawHit(){}
		virtual ~JFactoryGenerator_RawHit(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator_RawHit";}
		
		jerror_t GenerateFactories(jana::JEventLoop *loop){
			loop->AddFactory(new RawHit_factory());
			return NOERROR;
		}

};

#endif // _JFactoryGenerator_RawHit_

