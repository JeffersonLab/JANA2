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
#include <JANA/JQueue.h>
#include <JANA/JApplication.h>

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// JQueue subclasses
class JQueuePrimary:public JQueue{
	public:
		
		JQueuePrimary(void):JQueue("Primary", false){}
		~JQueuePrimary(void){}
};

class JQueueSecondary:public JQueue{
	public:
		
		JQueueSecondary():JQueue("Secondary", false){
			_convert_from_types.insert( "Primary" );
			japp->GetJQueue("Physics Events")->AddConvertFromType("Secondary");
		}
		~JQueueSecondary(){}
		
		int AddEvent(JEvent *jevent){
			AddToQueue(jevent); // in a real implementation we transform this in some way
			return 0;
		}
};


class JQueueFinal:public JQueue{
	public:
		
		JQueueFinal(void):JQueue("Final", false){
			_convert_from_types.insert( "Physics Events" );
			SetCanSink();
		}
		~JQueueFinal(void){}
		
		int AddEvent(JEvent *jevent){
			AddToQueue(jevent); // in a real implementation we transform this in some way
			return 0;
		}
};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


class JEventSource_jana_test: public JEventSource{
	public:
		JEventSource_jana_test(const char* source_name);
		virtual ~JEventSource_jana_test();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSource_jana_test";}
		
		RETURN_STATUS GetEvent(void);
		void FreeEvent(JEvent &event);
		void GetObjects(JEvent &event, JFactory *factory);
		
		JQueuePrimary *_queue_primary;
		JQueueSecondary *_queue_secondary;
		JQueueFinal *_queue_final;
};

#endif // _JEventSourceGenerator_jana_test_

