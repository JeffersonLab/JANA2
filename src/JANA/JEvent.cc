// $Id: JEvent.cc 1039 2005-06-14 20:21:02Z davidl $
//
//    File: JEvent.cc
// Created: Wed Jun  8 12:30:53 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include "JEvent.h"

//---------------------------------
// JEvent    (Constructor)
//---------------------------------
JEvent::JEvent()
{
	source = NULL;
	event_number = 0 ;
	run_number = 0;
	ref = NULL;
}

//---------------------------------
// ~JEvent    (Destructor)
//---------------------------------
JEvent::~JEvent()
{

}

//---------------------------------
// Print
//---------------------------------
void JEvent::Print(void)
{
	cout<<"JEvent: this=0x"<<hex<<(unsigned long)this<<dec;
	cout<<" source=0x"<<hex<<(unsigned long)source<<dec;
	cout<<" event_number="<<event_number;
	cout<<" run_number="<<run_number;
	cout<<" ref=0x"<<hex<<(unsigned long)ref<<dec;
	cout<<" loop=0x"<<hex<<(unsigned long)loop<<dec;
	cout<<endl;
}
