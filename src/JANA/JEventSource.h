// $Id: JEventSource.h 1098 2005-07-13 19:47:49Z davidl $
//
//    File: JEventSource.h
// Created: Wed Jun  8 12:31:04 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JEventSource_
#define _JEventSource_

#include <stdint.h>

#include <vector>
#include <string>
#include <set>
using std::vector;
using std::string;
using std::set;

#include <pthread.h>

#include "jerror.h"

// The following is here just so we can use ROOT's THtml class to generate
// documentation.
#if defined(__CINT__) || defined(__CLING__)
class pthread_mutex_t;
#endif


// Place everything in JANA namespace
namespace jana{

class JFactory_base;
class JEvent;


/// This is the base class for event sources in JANA. See
/// JEventSourceGenerator for a brief overview of how sources
/// are implemented in JANA.
///
/// To implement a new type of event source in JANA, one must
/// provide two classes. One that inherits from JEventSourceGenerator
/// and one that inherits from JEventSource. The JEventSource 
/// based class does the real work of reading data from the source
/// and creating objects that it passes back to the framework.
///
/// There are three methods that the subclass must provide to
/// implement an event source:
///
/// <ul>
/// <li> GetEvent:
/// 	This should read in the next event from the source and fill
/// 	in the JEvent objects fields so that it can be referred to 
/// 	later. Note that the entire event doesn't necessarily have
/// 	to be read in here. The framework is designed so that one
///	    could read in just a header and have the JEvent refer to 
/// 	the header info. Then, use that to jump to the specific data
/// 	in the source when GetObjects is called. If another event
/// 	is successfully read in, then NOERROR should be returned
/// 	(defined in jerror.h). Otherwise, NO_MORE_EVENTS_IN_SOURCE
/// 	should be returned.
///
/// <li> FreeEvent:
/// 	This gets called once the event has been fully processed and
/// 	is no longer needed. Any memory associated with the event
/// 	can then be freed.
///
/// <li> GetObjects:
/// 	This is where most of the work is probably done. When the
/// 	framework receives a request for objects of a certain type/tag,
/// 	it will first try and find the factory corresponding to it.
///	    The factory is needed only as a reservior for the objects.
/// 	If an appropriate factory is not found, one will be created
///	    automatically. This allows one to read in objects of 
///	    any type without having to explicitly make a factory for
///	    them.
///
///	    The factory pointer is passed so that its CopyTo(...) method
///	    may be called to insert the objects created by the event source
///	    into it. Thereby passing ownership to the factory.
///
///	    In the GetObjects method, a check should first be made on
///	    what class of object is requested. Since the source author
///	    presumably knows the classes their source can
///	    provide, a finite  and known number of checks are needed.
/// 	Perhaps the best way to do this is through a dynamic_cast of
/// 	the factory pointer with a NULL check like this:
///
/// 	<pre>
/// 	JFactory<DADC> fac = dynamic_cast<JFactory<DADC>*>(factory);
/// 	if(fac != NULL){
///		.... create DADC objects from event and copy them
///		into the factory with fac->CopyTo(...) ...
/// 	}
/// 	</pre>
/// </ul>
///


class JEventSource{
	public:
		JEventSource(const char *source_name); ///< Constructor
		virtual ~JEventSource(); ///< Destructor
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource";}
		
		virtual jerror_t GetEvent(JEvent &event);   ///< Get the next event from this source
		virtual void FreeEvent(JEvent &event);      ///< Free an event that is no longer needed.
		virtual jerror_t GetObjects(JEvent &event, JFactory_base *factory); ///< Get objects from an event

		inline const char* GetSourceName(void){return source_name.c_str();} ///< Get this sources name
		bool IsFinished(void);

	protected:
		string source_name;
		int source_is_open;
		pthread_mutex_t read_mutex;
		pthread_mutex_t in_progress_mutex;
		int Nevents_read;
		bool done_reading; // set to true after all events have been read (determined by return value from call to GetEvent() )
		set<uint64_t> in_progess_events;
		uint64_t Ncalls_to_GetEvent;

		inline void LockRead(void){pthread_mutex_lock(&read_mutex);}
		inline void UnlockRead(void){pthread_mutex_unlock(&read_mutex);}
	
	private:

};

} // Close JANA namespace

#endif // _JEventSource_

