// $Id$
//
//    File: JEventProcessor_jana_test.cc
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include <iostream>
#include <algorithm>

#include <JANA/JApplication.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "JEventSource_jana_test.h"
#include "JEventProcessor_jana_test.h"
#include "JFactoryGenerator_jana_test.h"

#include "jana_test.h"
#include "JEvent.h"
#include "JLog.h"
#include "JSourceObject.h"
#include "JQueueWithLock.h"

// Routine used to create our JEventProcessor
#include <JANA/JApplication.h>
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->Add(new JEventSourceGeneratorT<JEventSource_jana_test>());
	app->Add(new JFactoryGenerator_jana_test());
	app->Add(new JEventProcessor_jana_test());
}
} // "C"

//------------------
// JEventProcessor_jana_test (Constructor)
//------------------
JEventProcessor_jana_test::JEventProcessor_jana_test(void)
{
	//This is the new init()

	//Add queue for subtasks (not supplied by default!)
	auto sSubtaskQueue = new JQueueWithLock("Subtasks", 2000);
	japp->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, sSubtaskQueue);
}

//------------------
// ~JEventProcessor_jana_test (Destructor)
//------------------
JEventProcessor_jana_test::~JEventProcessor_jana_test(void)
{
	//This is the new fini()
	std::cout << "Total # objects = " << mNumObjects << "\n";
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
	mNumObjects += std::distance(sIterators_JanaTest.first, sIterators_JanaTest.second);
	auto sIterators_SourceObject = aEvent->Get<JSourceObject>(); //Will get from file
	mNumObjects += std::distance(sIterators_SourceObject.first, sIterators_SourceObject.second);
	auto sIterators_SourceObject2 = aEvent->Get<JSourceObject2>(); //Will get from file, and will submit jobs to generate random #'s
	mNumObjects += std::distance(sIterators_SourceObject2.first, sIterators_SourceObject2.second);
/*
	//Print jana_test objects hd_dump-style
		//(arg = 2 (should probably replace with enum))
	JLog sDumpLogger(2); //all objects at once: this way other threads can't interleave output in between rows
	sDumpLogger << "Thread " << JTHREAD->GetThreadID() << " JEventProcessor_jana_test::AnalyzeEvent(): jana_test's:\n" << JLogEnd();
	while(sIterators_JanaTest.first != sIterators_JanaTest.second)
	{
		auto& sObject = *(sIterators_JanaTest.first);
		sDumpLogger << sObject;
		sIterators_JanaTest.first++;
	}
	sDumpLogger << JLogEnd(); //only now is it dumped to screen

	//Print JSourceObject's inline-style
	JLog sPrintLogger(0);
	sPrintLogger << "Thread " << JTHREAD->GetThreadID() << " JEventProcessor_jana_test::AnalyzeEvent(): JSourceObject's:\n" << JLogEnd();
	while(sIterators_SourceObject.first != sIterators_SourceObject.second)
	{
		//Can do this in one line, but being explicit to make it easier to read
		auto& sObject = *(sIterators_SourceObject.first);
		sPrintLogger << sObject << "\n";
		sIterators_SourceObject.first++;
	}
	sPrintLogger << JLogEnd();

	//Print JSourceObject2's inline-style
	sPrintLogger << "Thread " << JTHREAD->GetThreadID() << " JEventProcessor_jana_test::AnalyzeEvent(): JSourceObject2's:\n" << JLogEnd();
	while(sIterators_SourceObject2.first != sIterators_SourceObject2.second)
		sPrintLogger << *(sIterators_SourceObject2.first++) << "\n"; //One-liner version
	sPrintLogger << JLogEnd();
	*/
}
