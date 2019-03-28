#ifndef _JEventProcessor_jana_test_
#define _JEventProcessor_jana_test_

#include <atomic>
#include <iostream>
#include <algorithm>

#include "JApplication.h"
#include "JTestEventSource.h"
#include "JEvent.h"
#include "JLogger.h"
#include "JTestDataObject.h"
#include "JQueueWithLock.h"


#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JTestEventProcessor : public JEventProcessor{

	public:

		JTestEventProcessor(JApplication* app) : JEventProcessor(app) {

			//Add queue for subtasks (not supplied by default!)
			auto sSubtaskQueue = new JQueueWithLock(app->GetJParameterManager(), "Subtasks", 2000);
			app->GetJThreadManager()->AddQueue(JQueueSet::JQueueType::SubTasks, sSubtaskQueue);
		}

		~JTestEventProcessor() {
			std::cout << "Total # objects = " << mNumObjects <<  std::endl;
		}

		const char* className() {
			return "JTestEventProcessor";
		}

		void Init(void) {
			std::cout << "JTestEventProcessor::Init() called" << std::endl;
		}

		void Process(const std::shared_ptr<const JEvent>& aEvent) {
			// Grab all objects, but don't do anything with them. The jana_test factory
			// will also grab the two types of source objects and then so some busy work
			// to use up CPU.

			auto sIterators_JanaTest = aEvent->Get<JTestDataObject>(); //Will get from factory
			auto sIterators_SourceObject = aEvent->Get<JTestSourceData1>(); //Will get from file
			auto sIterators_SourceObject2 = aEvent->Get<JTestSourceData2>(); //Will get from file, and will submit jobs to generate random #'s

			mNumObjects += std::distance(sIterators_JanaTest.first, sIterators_JanaTest.second);
			mNumObjects += std::distance(sIterators_SourceObject.first, sIterators_SourceObject.second);
			mNumObjects += std::distance(sIterators_SourceObject2.first, sIterators_SourceObject2.second);
		}

		void Finish() {
			std::cout << "JTestEventProcessor::Finish() called" << std::endl;
		}

	private:
		std::atomic<std::size_t> mNumObjects{0};
};

#endif // _JEventProcessor_jana_test_

