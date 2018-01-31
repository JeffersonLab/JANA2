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

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>
#include <JANA/JQueue.h>
#include <JANA/JApplication.h>
#include "JResourcePool.h"

#include "JEvent_test.h"

class JEventSource_jana_test: public JEventSource
{
	public:
		JEventSource_jana_test(const char* source_name);
		virtual ~JEventSource_jana_test();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_jana_test";}
		
		void Open(void);
		bool GetObjects(const std::shared_ptr<JEvent>& aEvent, JFactoryBase* aFactory);

		std::pair<std::shared_ptr<JEvent>, RETURN_STATUS> GetEvent(void);
		std::type_index GetDerivedType(void) const{return std::type_index(typeid(JEventSource_jana_test));} //So that we only execute factory generator once per type

	protected:
		std::size_t mNumEventsToGenerate;
		std::vector<std::shared_ptr<JEvent_test>> mEventsFromFile; //simulates a file

		//Resource pool for events
		JResourcePool<JEvent_test> mEventPool;
};

#endif // _JEventSourceGenerator_jana_test_

