// $Id$
//
//    File: JEventSource_MySource.h
// Created: Mon Dec 20 08:18:56 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#ifndef _JEventSource_MySource_
#define _JEventSource_MySource_

#include <JANA/jerror.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

class JEventSource_MySource: public jana::JEventSource{
	public:
		JEventSource_MySource(const char* source_name);
		virtual ~JEventSource_MySource();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_MySource";}
		
		jerror_t GetEvent(jana::JEvent &event);
		void FreeEvent(jana::JEvent &event);
		jerror_t GetObjects(jana::JEvent &event, jana::JFactory_base *factory);
};

#endif // _JEventSourceGenerator_MySource_

