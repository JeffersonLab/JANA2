// $Id$
//
//    File: JEventxample1.cc
// Created: Wed Jan  3 12:37:53 EST 2018
// Creator: davidl (on Linux jana2.jlab.org 3.10.0-693.11.1.el7.x86_64)
//

#include <memory>
#include <utility>

#include <JANA/JApplication.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JEvent.h>


//-------------------------------------------------------------------------
// This class represents a single, complete "event" read from the source.
// The only requirement here is that it inherit from JObject.
class JEventContext: public JObject {
	public:	
		double A;
		int B;		
};


//-------------------------------------------------------------------------
// This class would be responsible for opening and reading from the source
// of events (e.g. a file or network socket). It should read an event every
// time the GetEvent() method is called. The actual data read should be
// encapsulated in the form of a JEvent object.
class JEventSource_example1: public JEventSource {
	public:
	
		// Constructor must take string and JApplication pointer as arguments
		// and pass them into JEventSource constructor.
		JEventSource_example1(std::string source_name, JApplication *app):JEventSource(source_name, app){}
	
		// A description of this source type must be provided as a static member
		static std::string GetDescription(void){ return "Event source for JExample1"; }

		// This method is called to read in a single "event"
		void GetEvent(std::shared_ptr<JEvent> event) {
		
			// Throw exception if we have exhausted the source of events.
			static size_t Nevents = 0; // by way of example, just count 1000000 events
			if( ++Nevents > 1000000 ) throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
			
			// Create a JEvent object and fill in important info
			auto jevent_context = new JEventContext();
			jevent_context->A = 1.0;
			jevent_context->B = 2;

			event->Insert(jevent_context);
		}
};

//-------------------------------------------------------------------------
// This little piece of code is executed when the plugin is attached.
// It should always call "InitJANAPlugin(app)" first, and then register
// any objects it needs to with the system. In this case, we only need
// to create and register a JEventSourceGenerator_example1 object.
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	
	app->Add( new JEventSourceGeneratorT<JEventSource_example1>() );
}
} // "C"

