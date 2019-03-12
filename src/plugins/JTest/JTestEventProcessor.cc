// $Id$
//
//    File: JEventProcessor_jana_test.cc
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include <iostream>
#include <algorithm>

#include <JApplication.h>

#include "JTestEventSource.h"
#include "JTestEventProcessor.h"

#include "JTestDummyData.h"
#include "JEvent.h"
#include "JLogger.h"
#include "JTestSourceData1.h"
#include "JQueueWithLock.h"


//------------------
// JEventProcessor_jana_test (Constructor)
//------------------
JTestEventProcessor::JTestEventProcessor(JApplication* aApp) : JEventProcessor(aApp)
{
	//Add queue for subtasks (not supplied by default!)
	auto sSubtaskQueue = new JQueueWithLock(aApp->GetJParameterManager(), "Subtasks", 2000);
	aApp->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, sSubtaskQueue);
}

//------------------
// ~JEventProcessor_jana_test (Destructor)
//------------------
JTestEventProcessor::~JTestEventProcessor(void)
{
	std::cout << "Total # objects = " << mNumObjects <<  std::endl;
}

//------------------
// Init
//------------------
void JTestEventProcessor::Init(void)
{
	std::cout << "JEventProcessor_jana_test::Init() called" << std::endl;
}

//------------------
// Process
//------------------
void JTestEventProcessor::Process(const std::shared_ptr<const JEvent>& aEvent)
{

	// Check if this event came from the JTest event source. If not, then assume
	// the user is testing their own code and return right away.
	auto event_source_type = aEvent->GetEventSource()->GetType();
	if( event_source_type != "JTestEventSource" ) return;

	// Grab all objects, but don't do anything with them. The jana_test factory
	// will also grab the two types of source objects and then so some busy work
	// to use up CPU.

	// Get objects
	auto sIterators_JanaTest = aEvent->Get<JTestDummyData>(); //Will get from factory
	auto sIterators_SourceObject = aEvent->Get<JTestSourceData1>(); //Will get from file
	auto sIterators_SourceObject2 = aEvent->Get<JTestSourceData2>(); //Will get from file, and will submit jobs to generate random #'s
	mNumObjects += std::distance(sIterators_JanaTest.first, sIterators_JanaTest.second);
	mNumObjects += std::distance(sIterators_SourceObject.first, sIterators_SourceObject.second);
	mNumObjects += std::distance(sIterators_SourceObject2.first, sIterators_SourceObject2.second);
}

//------------------
// Finish
//------------------
void JTestEventProcessor::Finish(void)
{
	std::cout << "JEventProcessor_jana_test::Finish() called" << std::endl;
}
