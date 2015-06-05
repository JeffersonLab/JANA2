// $Id$
//
//    File: JEventSource_NULL.h
// Created: Thu Jun  4 15:52:36 EDT 2015
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventSource_NULL_
#define _JEventSource_NULL_

#include <JANA/jerror.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

class JEventSource_NULL: public jana::JEventSource{
	public:
		JEventSource_NULL(const char* source_name):JEventSource(source_name){}
		virtual ~JEventSource_NULL(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_NULL";}
		
		//----------------
		// GetEvent
		//----------------
		jerror_t GetEvent(jana::JEvent &event){
			event.SetJEventSource(this);
			event.SetEventNumber(++Nevents_read);
			event.SetRunNumber(1);
			event.SetRef(NULL);
			return NOERROR;
		}

		//----------------
		// FreeEvent
		//----------------
		void FreeEvent(jana::JEvent &event){}

		//----------------
		// GetObjects
		//----------------
		jerror_t GetObjects(jana::JEvent &event, jana::JFactory_base *factory){
			return OBJECT_NOT_AVAILABLE;
		}
};

#endif // _JEventSourceGenerator_NULL_

