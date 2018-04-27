// $Id$
//
//    File: JEventxample1.cc
// Created: Wed Jan  3 12:37:53 EST 2018
// Creator: davidl (on Linux jana2.jlab.org 3.10.0-693.11.1.el7.x86_64)
//

#include <memory>
#include <utility>

#include <JANA/JApplication.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JQueue.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>


//-------------------------------------------------------------------------
// This class represents a single, complete "event" read from the source.
// The only requirement here is that it inherit from JEvent.
class JEvent_example1:public JEvent {
	public:	

		double A;
		int B;		
};


//-------------------------------------------------------------------------
// This class would be responsible for opening and reading from the source
// of events (e.g. a file or network socket). It should read an event every
// time the GetEvent() method is called. The actual data read should be
// encapsulated in the form of a JEvent object.
class JEventSource_example1: public JEventSource{
	public:
		JEventSource_example1(const char* source_name):JEventSource(source_name){}

		// This method is called to read in a single "event"
		std::shared_ptr<const JEvent> GetEvent(void){
		
			// Throw exception if we have exhausted the source of events
			static size_t Nevents = 0; // by way of example, just count 1000000 events
			if( ++Nevents > 1000000 ) throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
			
			// Create a JEvent object and fill in important info
			auto jevent = new JEvent_example1();
			jevent->A = 1.0;
			jevent->B = 2;

			// Return the JEvent as a shared_ptr
			return std::shared_ptr<const JEvent>(jevent);
		}
};

//-------------------------------------------------------------------------
// This class is used to allow JANA to intelligently select which type
// of JEventSource object to make in order to read from a given source
// specified by the user. This allows multiple types of file formats
// or network sources to be supported in a single executable.
class JEventSourceGenerator_example1: public JEventSourceGenerator{
	public:
		JEventSourceGenerator_example1():JEventSourceGenerator("example1"){}
		
		const char* Description(void){ return "example1"; }

		// The value returned by CheckOpenable() should be between 0-1 with
		// larger values indicating greater confidence that this JEventSource
		// class can read from the given source. For this simple example, 0.5
		// is always returned and since it is the only JEventSourceGenerator
		// registered, it will always be chosen.
		double CheckOpenable(string source){ return 0.5; }

		// This is called to instantiate a JEventSource object
		JEventSource* MakeJEventSource(string source){ return new JEventSource_example1(source.c_str()); }
};

//-------------------------------------------------------------------------
// This little piece of code is executed when the plugin is attached.
// It should always call "InitJANAPlugin(app)" first, and then register
// any objects it needs to with the system. In this case, we only need
// to create and register a JEventSourceGenerator_example1 object.
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	
	auto source_generator = new JEventSourceGenerator_example1();
	app->GetJEventSourceManager()->AddJEventSourceGenerator( source_generator );
}
} // "C"

