// $Id$
//
//    File: JEventProcessor_jana_test.cc
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include <iostream>
#include <algorithm>

#include "JEventProcessor_jana_test.h"

#include "JEventSourceManager.h"
#include "JEventSourceGenerator_jana_test.h"
#include "JFactoryGenerator_jana_test.h"

#include "jana_test.h"
#include "JEvent.h"

// Routine used to create our JEventProcessor
#include <JANA/JApplication.h>
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->GetJEventSourceManager()->AddJEventSourceGenerator(new JEventSourceGenerator_jana_test());
	app->AddJFactoryGenerator(new JFactoryGenerator_jana_test());
	app->AddJEventProcessor(new JEventProcessor_jana_test());
}
} // "C"

//------------------
// JEventProcessor_jana_test (Constructor)
//------------------
JEventProcessor_jana_test::JEventProcessor_jana_test(void)
{
	//This is the new init()
}

//------------------
// ~JEventProcessor_jana_test (Destructor)
//------------------
JEventProcessor_jana_test::~JEventProcessor_jana_test(void)
{
	//This is the new fini()
}

//------------------
// ChangeRun
//------------------
void JEventProcessor_jana_test::ChangeRun(const std::shared_ptr<const JEvent>& aEvent)
{
	// This is called whenever the run number changes
}

//------------------
// AnalyzeEvent
//------------------
void JEventProcessor_jana_test::AnalyzeEvent(const std::shared_ptr<const JEvent>& aEvent)
{
	// This is called for every event. Use of common resources like writing
	// to a file or filling a histogram should be mutex protected. Using
	// loop->Get(...) to get reconstructed objects (and thereby activating the
	// reconstruction algorithm) should be done outside of any mutex lock
	// since multiple threads may call this method at the same time.

	auto sJanaTestIterators = aEvent->Get<jana_test>();

	auto sPrinter = [](const jana_test& aObject) -> void {std::cout << &aObject;};
	std::for_each(sJanaTestIterators.first, sJanaTestIterators.second, sPrinter);
}
