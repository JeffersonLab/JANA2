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
	//Add queue for subtasks (not supplied by default!)
	auto sSubtaskQueue = new JQueueWithLock("Subtasks", 2000);
	japp->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, sSubtaskQueue);
}

//------------------
// ~JEventProcessor_jana_test (Destructor)
//------------------
JEventProcessor_jana_test::~JEventProcessor_jana_test(void)
{
	std::cout << "Total # objects = " << mNumObjects <<  std::endl;
}

//------------------
// Init
//------------------
void JEventProcessor_jana_test::Init(void)
{
	std::cout << "JEventProcessor_jana_test::Init() called" << std::endl;
}

//------------------
// Process
//------------------
void JEventProcessor_jana_test::Process(const std::shared_ptr<const JEvent>& aEvent)
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
	mNumObjects += std::distance(sIterators_JanaTest.first, sIterators_JanaTest.second);
	mNumObjects += std::distance(sIterators_SourceObject.first, sIterators_SourceObject.second);
	mNumObjects += std::distance(sIterators_SourceObject2.first, sIterators_SourceObject2.second);
}

//------------------
// Finish
//------------------
void JEventProcessor_jana_test::Finish(void)
{
	std::cout << "JEventProcessor_jana_test::Finish() called" << std::endl;
}
