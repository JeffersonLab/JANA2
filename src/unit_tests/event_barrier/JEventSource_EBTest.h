// $Id$
//
//    File: JEventSource_EBTest.h
// Created: Tue Jun  9 11:28:14 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventSource_EBTest_
#define _JEventSource_EBTest_

#include <JANA/jerror.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

class JEventSource_EBTest: public jana::JEventSource{
	public:
		JEventSource_EBTest(const char* source_name):JEventSource(source_name){}
		virtual ~JEventSource_EBTest(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_EBTest";}
		
		jerror_t GetEvent(jana::JEvent &event){
			
			event.SetJEventSource(this);
			event.SetEventNumber(++Nevents_read);
			event.SetRunNumber(1234);
			event.SetRef(NULL);
			
			// Make every 150th event  barrier event
			if(Nevents_read%150 == 0) event.SetSequential();

			return NOERROR;
		}
		
		void FreeEvent(jana::JEvent &event){}		
		jerror_t GetObjects(jana::JEvent &event, jana::JFactory_base *factory){return OBJECT_NOT_AVAILABLE;}
};

#endif // _JEventSourceGenerator_EBTest_

