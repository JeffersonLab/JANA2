// $Id: JEvent.h 1042 2005-06-14 20:48:00Z davidl $
//
//    File: JEvent.h
// Created: Wed Jun  8 12:30:53 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JEvent_
#define _JEvent_

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
using std::vector;
using std::string;

#include "jerror.h"
#include "JEventSource.h"
#include "JFactory_base.h"


// Place everything in JANA namespace
namespace jana{

template<class T> class JFactory;

class JEvent{
	public:
		JEvent();
		virtual ~JEvent();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEvent";}
		template<class T> jerror_t GetObjects(vector<const T*> &t, JFactory_base *factory=NULL);
		inline JEventSource* GetJEventSource(void){return source;}
		inline int GetEventNumber(void){return event_number;}
		inline int GetRunNumber(void){return run_number;}
		inline void* GetRef(void){return ref;}
		inline JEventLoop* GetJEventLoop(void){return loop;}
		inline void SetJEventSource(JEventSource *source){this->source=source;}
		inline void SetRunNumber(int run_number){this->run_number=run_number;}
		inline void SetEventNumber(int event_number){this->event_number=event_number;}
		inline void SetRef(void *ref){this->ref=ref;}
		inline void SetJEventLoop(JEventLoop *loop){this->loop=loop;}
		inline void FreeEvent(void){if(source)source->FreeEvent(*this);}
		void Print(void);
	
	private:
		JEventSource *source;
		int event_number;
		int run_number;
		void *ref;
		JEventLoop *loop;

};


//---------------------------------
// GetObjects
//---------------------------------
template<class T>
jerror_t JEvent::GetObjects(vector<const T*> &t, JFactory_base *factory)
{
	/// Call the GetObjects() method of the associated source.
	/// This will cause the objects of the appropriate type and tag
	/// to be read in from the source and stored in the factory (if they
	/// exist in the source).
	
	// Make sure source is at least not NULL
	if(!source)throw EVENT_SOURCE_NOT_OPEN;
	
	// Get list of object pointers. This will read the objects in
	// from the source, instantiating them and handing ownership of
	// them over to the factory object. 
	jerror_t err = source->GetObjects(*this, factory);
	if(err != NOERROR)return err; // if OBJECT_NOT_AVAILABLE is returned, the source could not provide the objects
	
	// OK, must have found some objects (possibly even zero) in the source.
	// Copy the pointers into the passed reference. To do this we
	// need to cast the JFactory_base pointer into a JFactory pointer
	// of the proper flavor. This also adds a tiny layer of protection
	// in case the factory pointer passed doesn't correspond to a
	// JFactory based on type T.
	JFactory<T> *fac = dynamic_cast<JFactory<T>*>(factory);
	if(fac){
		fac->CopyFrom(t);
	}else{
		// If we get here, it means "factory" is not based on "T".
		// Actually, it turns out a long standing bug in g++ can
		// cause dynamic_cast to fail for objects created in a routine
		// attached via libdl. Because of this, we have to check the
		// name of the data class. I am leaving the above dynamic_cast
		// here because it *should* be the proper way to do this and
		// does actually work most of the time. Hopefully, this will
		// be resolved at some point and we can remove this message
		// and use this code to flag true bugs.
		if(!strcmp(factory->GetDataClassName(), T::static_className())){
			fac = (JFactory<T>*)factory;
			fac->CopyFrom(t);
		}else{
			std::cout<<__FILE__<<":"<<__LINE__<<" BUG DETECTED!! email davidl@jlab.org and complain!"<<std::endl;
			std::cout<<"factory->GetDataClassName()=\""<<factory->GetDataClassName()<<"\""<<std::endl;
			std::cout<<"T::className()=\""<<T::static_className()<<"\""<<std::endl;
		}
	}

	return NOERROR;
}

} // Close JANA namespace


#endif // _JEvent_

