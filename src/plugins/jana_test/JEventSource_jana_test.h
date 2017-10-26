// $Id$
//
//    File: JEventSource_jana_test.h
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventSource_jana_test_
#define _JEventSource_jana_test_

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

class JEventSource_jana_test: public JEventSource{
	public:
		JEventSource_jana_test(const char* source_name);
		virtual ~JEventSource_jana_test();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_jana_test";}
		
		void GetEvent(JEvent &event);
		void FreeEvent(JEvent &event);
		void GetObjects(JEvent &event, JFactory *factory);
};

#endif // _JEventSourceGenerator_jana_test_

