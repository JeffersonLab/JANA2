// Author: David Lawrence  June 25, 2004
//
//
// MyProcessor.cc
//

#include <iostream>
using namespace std;

#include <stdio.h>
#include <unistd.h>

#include "MyProcessor.h"


int PAUSE_BETWEEN_EVENTS = 1;
int SKIP_BORING_EVENTS = 0;
int PRINT_ALL=0;
extern bool SPARSIFY_SUMMARY;

vector<string> toprint;

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
// brun
//------------------------------------------------------------------
jerror_t MyProcessor::brun(JEventLoop *eventLoop, int runnumber)
{
	vector<string> factory_names;
	eventLoop->GetFactoryNames(factory_names);

	usleep(100000); //this just gives the Main thread a chance to finish printing the "Launching threads" message
	cout<<endl;

	// If int PRINT_ALL is set then add EVERYTHING.
	if(PRINT_ALL){
		toprint = factory_names;
		SKIP_BORING_EVENTS = 0; // with PRINT_ALL, nothing is boring!
	}else{
		// make sure factories exist for all requested data types
		// If a factory isn't found, but one with a "D" prefixed
		// is, go ahead and correct the name.
		vector<string> really_toprint;
		for(unsigned int i=0; i<toprint.size();i++){
			int found = 0;
			int dfound = 0;
			for(unsigned int j=0;j<factory_names.size();j++){
				if(factory_names[j] == toprint[i])found = 1;
				if(factory_names[j] == "D" + toprint[i])dfound = 1;
			}
			if(found)
				really_toprint.push_back(toprint[i]);
			else if(dfound)
				really_toprint.push_back("D" + toprint[i]);
			else
				cout<<ansi_red<<"WARNING:"<<ansi_normal
					<<" Couldn't find factory for \""
					<<ansi_bold<<toprint[i]<<ansi_normal
					<<"\"!"<<endl;
		}
		
		toprint = really_toprint;
	}
	
	// At this point, toprint should contain a list of all factories
	// in dataClassName:tag format, that both exist and were requested.
	// Seperate the tag from the name and fill the fac_info vector.
	fac_info.clear();
	for(unsigned int i=0;i<toprint.size();i++){
		string name = toprint[i];
		string tag = "";
		unsigned int pos = name.rfind(":",name.size()-1);
		if(pos != (unsigned int)string::npos){
			tag = name.substr(pos+1,name.size());
			name.erase(pos);
		}
		factory_info_t f;
		f.dataClassName = name;
		f.tag = tag;
		fac_info.push_back(f);
	}
	
	cout<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t MyProcessor::evnt(JEventLoop *eventLoop, int eventnumber)
{

	// If we're skipping boring events (events with no rows for any of
	// the types we're printing) we must find out first if the event is
	// "boring".
	int event_is_boring = 1;
	if(SKIP_BORING_EVENTS){
		for(unsigned int i=0;i<fac_info.size();i++){
			
			string name =fac_info[i].dataClassName;
			string tag = fac_info[i].tag;
			JFactory_base *factory = eventLoop->GetFactory(name,tag.c_str());
			if(factory){
				try{
					if(factory->GetNrows()>0){
						event_is_boring=0;
						break;
					}
				}catch(...){
					// someone threw an exception
				}
			}
		}
		if(event_is_boring)return NOERROR;
	}else{
		event_is_boring = 0;
	}
	
	// Print event separator
	cout<<"================================================================"<<endl;
	cout<<"Event: "<<eventnumber<<endl;

	// We want to print info about all factories results, even if we aren't
	// printing the actual data.
	eventLoop->PrintFactories(SPARSIFY_SUMMARY ? 1:0);
	
	// Print data for all specified factories
	for(unsigned int i=0;i<fac_info.size();i++){
		try{
			string name =fac_info[i].dataClassName;
			string tag = fac_info[i].tag;
			eventLoop->Print(name,tag.c_str());
		}catch(...){
			// exception thrown
		}
	}
	
	// Wait for user input if pausing
	if(PAUSE_BETWEEN_EVENTS && !event_is_boring){
		cerr.flush();
		cout<<endl<<"< Hit return for the next event (P=prev. Q=quit) >";
		cout.flush();
		char c = getchar(); // see note in janadump.cc:main()
		if(c=='\n')cout<<ansi_up(1);
		cout<<endl;
		switch(toupper(c)){
			case 'Q':
				eventLoop->Quit();
				break;
			case 'P':
				//eventLoop->GotoEvent(eventnumber-1);
				break;
			case 'N':
				break;
		}

		cout<<ansi_up(1)<<"\r                                                     \r";
		cout.flush();
	}
	
	return NOERROR;
}

