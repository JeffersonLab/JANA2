// $Id$
//
//    File: JEventxample1.cc
// Created: Wed Jan  3 12:37:53 EST 2018
// Creator: davidl (on Linux jana2.jlab.org 3.10.0-693.11.1.el7.x86_64)
//

#include <JANA/JApplication.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JEventSource.h>
#include <JANA/JQueue.h>


//-------------------------------------------------------------------------
// This class represents a single, complete "event" read from the source
class JEvent_example1:public JEvent {
	public:	
		JEvent_example1(double a, int b):A(a),B(b){}

		double A;
		int B;		
};


//-------------------------------------------------------------------------
// This class would be responsible for opening and reading from the source
// of events (e.g. a file or network socket). It should read one or more
// events every time the GetEvent() method is called. The actual data read
// should be encapsulated in a JEvent object and placed in the appropriate
// JQueue.
class JEventSource_example1: public JEventSource{
	public:
		JEventSource_example1(const char* source_name):JEventSource(source_name){}

		RETURN_STATUS GetEvent(void){
			JEvent *evt = new JEvent_example1(1.0, 2); // JANA will "free" this by calling its Recycle method
			japp->GetJQueue("Physics Events")->AddToQueue( evt );
			return kSUCCESS;
		}
};

//-------------------------------------------------------------------------
// This class is used to allow JANA to intelligently select which type
// of JEventSource object to make in order to read from a given source
// specified by the user. The value returned by CheckOpenable() should
// be between 0-1 with larger values indicating greater confidence that
// its JEventSource class can read from that source. For this simple 
// example, 0.5 is always returned and since it is the only JEventSourceGenerator
// registered, it will always be chosen.
class JEventSourceGenerator_example1: public JEventSourceGenerator{
	public:
		JEventSourceGenerator_example1():JEventSourceGenerator("example1"){}
		
		const char* Description(void){ return "example1"; }

		double CheckOpenable(string source){ return 0.5; }

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
	app->AddJEventSourceGenerator(new JEventSourceGenerator_example1());
}
} // "C"


