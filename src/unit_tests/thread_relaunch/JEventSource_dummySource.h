// $Id$
//
//    File: JEventSource_dummySource.h
// Created: Tue Jan 22 11:33:11 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _JEventSource_dummySource_
#define _JEventSource_dummySource_

#include <JANA/jerror.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

class JEventSource_dummySource: public jana::JEventSource{
	public:
		JEventSource_dummySource(const char* source_name);
		virtual ~JEventSource_dummySource();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_dummySource";}
		
		jerror_t GetEvent(jana::JEvent &event);
		void FreeEvent(jana::JEvent &event);
		jerror_t GetObjects(jana::JEvent &event, jana::JFactory_base *factory);
};

#endif // _JEventSourceGenerator_dummySource_

