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
class JEvent_example1:public JEvent {
	public:	
		JEvent_example1(double a, int b):A(a),B(b){}

		double A;
		int B;		
};


//-------------------------------------------------------------------------
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
class JEventSourceGenerator_example1: public JEventSourceGenerator{
	public:
		JEventSourceGenerator_example1():JEventSourceGenerator("example1"){}
		
		const char* Description(void){ return "example1"; }
		double CheckOpenable(string source){ return 0.5; }
		JEventSource* MakeJEventSource(string source){ return new JEventSource_example1(source.c_str()); }
};
//-------------------------------------------------------------------------

extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddJEventSourceGenerator(new JEventSourceGenerator_example1());
}
} // "C"


