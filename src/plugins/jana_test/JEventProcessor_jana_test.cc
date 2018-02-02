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
#include "JLog.h"
#include "JSourceObject.h"

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

	//Add queue for subtasks (not supplied by default!)
	japp->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, new JQueue("Subtasks", 2000));
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

	//Get objects
	auto sIterators_JanaTest = aEvent->Get<jana_test>(); //Will get from factory
	auto sIterators_SourceObject = aEvent->Get<JSourceObject>(); //Will get from file
	auto sIterators_SourceObject2 = aEvent->Get<JSourceObject2>(); //Will get from file, and will submit jobs to generate random #'s

	//Print JSourceObject's inline-style
	while(sIterators_SourceObject.first != sIterators_SourceObject.second)
	{
		//Can do this in one line, but being explicit to make it easier to read
		auto& sObject = *(sIterators_SourceObject.first);
		JLog() << sObject << JLogEnd(); //One object at a time
		sIterators_SourceObject.first++;
	}

	//Print JSourceObject2's inline-style
	while(sIterators_SourceObject2.first != sIterators_SourceObject2.second)
		JLog() << *(sIterators_SourceObject2.first++) << JLogEnd(); //One-liner version

	//Print jana_test objects hd_dump-style
		//(arg = 2 (should probably replace with enum))
	JLog sDumpLogger(2); //all objects at once: this way other threads can't interleave output in between rows
	while(sIterators_JanaTest.first != sIterators_JanaTest.second)
	{
		auto& sObject = *(sIterators_JanaTest.first);
		sDumpLogger << sObject;
		sIterators_JanaTest.first++;
	}
	sDumpLogger << JLogEnd(); //only now is it dumped to screen
}
