// $Id$
//
//    File: JEventProcessor_UserReference.cc
// Created: Tue Nov 26 10:49:45 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 13.0.0 i386)
//

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>

#include "JEventProcessor_UserReference.h"
using namespace jana;


//------------------
// JEventProcessor_UserReference (Constructor)
//------------------
JEventProcessor_UserReference::JEventProcessor_UserReference()
{
	pthread_mutex_init(&mutex, NULL);
}

//------------------
// ~JEventProcessor_UserReference (Destructor)
//------------------
JEventProcessor_UserReference::~JEventProcessor_UserReference()
{

}

//------------------
// init
//------------------
jerror_t JEventProcessor_UserReference::init(void)
{
	// This is called once at program startup
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_UserReference::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	// This is called whenever the run number changes
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_UserReference::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	// Get the reference to the counter if it exists. If not, then create
	// a counter and store it as a user reference. This should create one
	// for every thread.
	int *icntr = loop->GetRef<int>();
	if(!icntr){
		icntr = new int;
		*icntr = 0;
		loop->SetRef(icntr);

		// Keep private list so we can access results later
		pthread_mutex_lock(&mutex);
		icounters.insert(icntr);
		pthread_mutex_unlock(&mutex);
	}

	float *fcntr = loop->GetRef<float>();
	if(!fcntr){
		fcntr = new float;
		*fcntr = 0.0;
		loop->SetRef(fcntr);

		// Keep private list so we can access results later
		pthread_mutex_lock(&mutex);
		fcounters.insert(fcntr);
		pthread_mutex_unlock(&mutex);
	}

	// increment counters
	(*icntr)++;
	(*fcntr) += 1.0;

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_UserReference::erun(void)
{

	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_UserReference::fini(void)
{
	// Print final results to screen
	GetTotalI(true);
	GetTotalF(true);

	return NOERROR;
}

//------------------
// GetTotalI
//------------------
int JEventProcessor_UserReference::GetTotalI(bool print_vals)
{
	// Get total of all counters. If the print_vals is true,
	// the the values are printed as the total is tallied.

	if(print_vals){
		cout << endl;
		cout<<"Checking int loop counters"<<endl;
		cout<<"-----------------------"<<endl;
	}

	// Get list of all user references
	int itot = 0;
	set<int*>::iterator iter;
	for(iter=icounters.begin(); iter!=icounters.end(); iter++){
		int *icntr = *iter;
		if(print_vals) cout << " icntr="<<icntr<<"   ";
		if(icntr){
			if(print_vals) cout<<*icntr;
			itot += *icntr;
		}
		if(print_vals) cout <<endl;
	}

	if(print_vals){
		cout << "   total:  " << itot << endl;
	}

	return itot;
}

//------------------
// GetTotalF
//------------------
float JEventProcessor_UserReference::GetTotalF(bool print_vals)
{
	// Get total of all counters. If the print_vals is true,
	// the the values are printed as the total is tallied.

	if(print_vals){
		cout << endl;
		cout<<"Checking float loop counters"<<endl;
		cout<<"-----------------------"<<endl;
	}

	// Get list of all user references
	int ftot = 0;
	set<float*>::iterator iter;
	for(iter=fcounters.begin(); iter!=fcounters.end(); iter++){
		float *fcntr = *iter;
		if(print_vals) cout << " fcntr="<<fcntr<<"   ";
		if(fcntr){
			if(print_vals) cout<<*fcntr;
			ftot += *fcntr;
		}
		if(print_vals) cout <<endl;
	}

	if(print_vals){
		cout << "   total:  " << ftot << endl;
	}

	return ftot;
}


