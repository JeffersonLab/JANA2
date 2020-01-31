// $Id$
//
//    File: JEventxample2.cc
// Created: Thu Apr 26 22:07:34 EDT 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.5.0 x86_64)
//


#include <JANA/JApplication.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "JEventProcessor_example2.h"
#include "JEventSource_example2.h"
#include "JFactoryGenerator_example2.h"


//-------------------------------------------------------------------------
// This little piece of code is executed when the plugin is attached.
// It should always call "InitJANAPlugin(app)" first, and then register
// any objects it needs to with the system. In this case, we need
// to register one each of a JEventSourceGeneratorT<JEventSource_example2>,
// a JFactoryGenerator_example2, and a JEventProcessor_example2 object.
extern "C"{
void InitPlugin(JApplication *app){

	InitJANAPlugin(app);
	
	// Add source generator
	app->Add( new JEventSourceGeneratorT<JEventSource_example2>() );

	// Add factory generator
	app->Add( new JFactoryGenerator_example2() );

	// Add event processor
	app->Add( new JEventProcessor_example2() );

}
} // "C"

