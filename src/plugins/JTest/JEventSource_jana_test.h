// $Id$
//
//    File: JEventSource_jana_test.h
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventSource_jana_test_
#define _JEventSource_jana_test_

#include <vector>
#include <memory>
#include <random>

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>
#include <JANA/JQueueSimple.h>
#include <JANA/JApplication.h>
#include "JResourcePool.h"

#include "JEvent_test.h"
#include "JSourceObject.h"
#include "JSourceObject2.h"

class JEvent;

class JEventSource_jana_test: public JEventSource
{
	public:
		JEventSource_jana_test(string source_name, JApplication *japp);
		virtual ~JEventSource_jana_test();


		static std::string GetDescription(void) { return "JTest Fake Event Source"; }
		std::string GetType(void) const {return GetDemangledName<decltype(*this)>();}
		void Open(void);

		//Public get interface
		bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory);

		void GetEvent(std::shared_ptr<JEvent>);
		std::type_index GetDerivedType(void) const{return std::type_index(typeid(JEventSource_jana_test));} //So that we only execute factory generator once per type

	private:

		//Getter functions for each type
		bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactoryT<JSourceObject>* aFactory);
		bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactoryT<JSourceObject2>* aFactory);

		void LockGenerator(void) const;

		std::size_t mNumEventsToGenerate;
		std::size_t mNumEventsGenerated = 0;
		std::mt19937 mRandomGenerator;
		mutable std::atomic<bool> mGeneratorLock{false};

		int mDebugLevel = 0;
		int mLogTarget = 0; //std::cout
		bool mIncludeBarriers = true;

		//Resource pool for events
		JResourcePool<JEvent_test> mEventPool;
};

#endif // _JEventSourceGenerator_jana_test_

