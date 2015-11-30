// Author: Edward Brash February 15, 2005
// revised severely 2006-2007  David Lawrence
//
//
// MyProcessor.cc
//

#include <iostream>
using namespace std;

#include "MyProcessor.h"

map<string, string> autoactivate;
bool ACTIVATE_ALL=false;

#define ansi_escape		((char)0x1b)
#define ansi_bold 		ansi_escape<<"[1m"
#define ansi_black		ansi_escape<<"[30m"
#define ansi_red			ansi_escape<<"[31m"
#define ansi_green		ansi_escape<<"[32m"
#define ansi_blue			ansi_escape<<"[34m"
#define ansi_normal		ansi_escape<<"[0m"
#define ansi_up(A)		ansi_escape<<"["<<(A)<<"A"
#define ansi_down(A)		ansi_escape<<"["<<(A)<<"B"
#define ansi_forward(A)	ansi_escape<<"["<<(A)<<"C"
#define ansi_back(A)		ansi_escape<<"["<<(A)<<"D"


//------------------------------------------------------------------
// init 
//------------------------------------------------------------------
jerror_t MyProcessor::init(void)
{
	// Nothing to do here.

	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t MyProcessor::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	// Print factory names
	eventLoop->PrintFactories();

	// Get list of factory names in name:tag format
	map<string, string> factorynames;
	eventLoop->GetFactoryNames(factorynames);

	// If ACTIVATE_ALL is set then add EVERYTHING.
	if(ACTIVATE_ALL){
		autoactivate=factorynames;
	}
	
	// Make sure factories exist for all requested data types. Print
	// warnings for those not found. For those that are, split the
	// name from "name:tag" format into separate name and tag strings
	// and copy them into the autofactories map.
	map<string, string>::iterator iter = autoactivate.begin();
	for(; iter!=autoactivate.end(); iter++){
		bool found = false;
		map<string, string>::iterator iter2 = factorynames.find(iter->first);
		if(iter2!=factorynames.end()){
			if(iter->second==iter2->second){
				autofactories[iter->first] = iter->second;
				found = true;
			}
		}
		if(!found){
			cout<<ansi_red<<"WARNING:"<<ansi_normal
				<<" Couldn't find factory for \""
				<<ansi_bold<<iter->first;
			if(iter->second!="")cout<<":"<<iter->second;
			cout<<ansi_normal<<"\"!"<<endl;
		}
	}

	cout<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t MyProcessor::evnt(JEventLoop *eventLoop, uint64_t eventnumber)
{
	// Loop over factories explicitly mentioned on command line
	map<string, string>::iterator iter = autofactories.begin();
	for(; iter!=autofactories.end(); iter++){
		const char *name = iter->first.c_str();
		const char *tag = iter->second.c_str();
		JFactory_base *factory = eventLoop->GetFactory(name,tag);
		if(factory){
			try{
				factory->GetNrows();
			}catch(...){
				// someone threw an exception
			}
		}
	}

	return NOERROR;
}

//------------------------------------------------------------------
// fini   -Close output file here
//------------------------------------------------------------------
jerror_t MyProcessor::fini(void)
{
	return NOERROR;
}

