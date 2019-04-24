// $Id$
//
//    File: JEventProcessor_jana_test.cc
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include <iostream>
#include <algorithm>

#include <JApplication.h>

#include "JEventSource_jana_test.h"
#include "JEventProcessor_jana_test.h"

#include "jana_test.h"
#include "JEvent.h"
#include "JLogger.h"
#include "JSourceObject.h"
#include "JQueueWithLock.h"


//------------------
// JEventProcessor_jana_test (Constructor)
//------------------
JEventProcessor_jana_test::JEventProcessor_jana_test(JApplication* aApp) : JEventProcessor(aApp)
{
	//Add queue for subtasks (not supplied by default!)
	auto sSubtaskQueue = new JQueueWithLock(aApp->GetJParameterManager(), "Subtasks", 2000);
	aApp->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, sSubtaskQueue);
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

	// Check if this event came from the JTest event source. If not, then assume
	// the user is testing their own code and return right away.
	auto event_source_type = aEvent->GetEventSource()->GetType();
	if( event_source_type != "JEventSource_jana_test" ) return;

	// Grab all objects, but don't do anything with them. The jana_test factory
	// will also grab the two types of source objects and then so some busy work
	// to use up CPU.

	// Get objects
	auto sIterators_JanaTest = aEvent->GetIterators<jana_test>(); //Will get from factory
	auto sIterators_SourceObject = aEvent->GetIterators<JSourceObject>(); //Will get from file
	auto sIterators_SourceObject2 = aEvent->GetIterators<JSourceObject2>(); //Will get from file, and will submit jobs to generate random #'s
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
