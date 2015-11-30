// Author: David Lawrence  June 25, 2004
//
//
// MyProcessor.cc
//

#include <iostream>
using namespace std;

#include <stdio.h>
#include <unistd.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JApplication.h>

#include "MyProcessor.h"


int PAUSE_BETWEEN_EVENTS = 1;
int SKIP_BORING_EVENTS = 0;
int PRINT_ALL=0;
bool LIST_ASSOCIATED_OBJECTS = false;
bool PRINT_SUMMARY_HEADER = true;
bool PRINT_STATUS_BITS = false;
bool ACTIVATE_TAGGED_FOR_SUMMARY = false;
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
jerror_t MyProcessor::brun(JEventLoop *eventLoop, int32_t runnumber)
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
		f.fac = eventLoop->GetFactory(f.dataClassName, f.tag.c_str());
		fac_info.push_back(f);
	}
	
	cout<<endl;

	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t MyProcessor::evnt(JEventLoop *eventLoop, uint64_t eventnumber)
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
			if(!factory)factory = eventLoop->GetFactory("D" + name,tag.c_str());
			if(factory){
				try{
					if(factory->GetNrows()>0){
						event_is_boring=0;
						if(PRINT_SUMMARY_HEADER)break;
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
	
	if(SKIP_BORING_EVENTS && event_is_boring)return NOERROR;
	if(!SKIP_BORING_EVENTS)event_is_boring= 0;
	
	// Print event separator
	cout<<"================================================================"<<endl;
	cout<<"Event: "<<eventnumber<<endl;

	// We want to print info about all factories results, even if we aren't
	// printing the actual data. To make sure the informational messages often
	// printed during brun are printed first, call the GetNrows() method of
	// all factories ourself first.
	if(PRINT_SUMMARY_HEADER){
		vector<JFactory_base*> myfacs = eventLoop->GetFactories();
		for(unsigned int i=0; i<myfacs.size(); i++){
			string tag = myfacs[i]->Tag()==NULL ? "":myfacs[i]->Tag();
			if(tag=="" || ACTIVATE_TAGGED_FOR_SUMMARY){
				myfacs[i]->GetNrows();
			}
		}
	}
	
	if(PRINT_STATUS_BITS) cout << japp->StatusWordToString(eventLoop->GetJEvent().GetStatus()) << endl;
	if(PRINT_SUMMARY_HEADER) eventLoop->PrintFactories(SPARSIFY_SUMMARY ? 2:0);
	
	// Print data for all specified factories
	for(unsigned int i=0;i<fac_info.size();i++){
		try{
			string name =fac_info[i].dataClassName;
			string tag = fac_info[i].tag;
			eventLoop->Print(name,tag.c_str());
			if(LIST_ASSOCIATED_OBJECTS)PrintAssociatedObjects(eventLoop, &fac_info[i]);
		}catch(...){
			// exception thrown
		}
	}
	
	// If the program is quitting, then don't bother waiting for the user
	if(eventLoop->GetJApplication()->GetQuittingStatus())return NOERROR;
	
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
				eventLoop->QuitProgram();
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

//------------------------------------------------------------------
// PrintAssociatedObjects
//------------------------------------------------------------------
void MyProcessor::PrintAssociatedObjects(JEventLoop *loop, const factory_info_t *fac_info)
{
	// cast away const-ness of JFactory_base class pointer
	JFactory_base *fac = const_cast<JFactory_base*>(fac_info->fac);
	if(!fac)return;

	// Get list of all objects from this factory
	vector<void*> vobjs = fac->Get();
	vector<JObject*> objs;
	for(unsigned int i=0; i<vobjs.size(); i++)objs.push_back((JObject*)vobjs[i]);
	
	// Loop over objects from this factory
	for(unsigned int i=0; i<objs.size(); i++){
	
		// First, get a list of all associated objects
		vector<const JObject*> aobjs;
		objs[i]->GetT(aobjs);
		// If no associated objects, just go on to the next object
		if(aobjs.size()==0)continue;
		
		// Print separator
		cout<<"  [== Associated objects for row "<<i<<" ==]"<<endl;

		// Make a list of all factories that made objects associated to this one
		map<JFactory_base*, vector<const JObject*> > aofacs;
		for(unsigned int j=0; j<aobjs.size(); j++){
			JFactory_base *aofac = loop->FindOwner(aobjs[j]);
			
			map<JFactory_base*, vector<const JObject*> >::iterator iter = aofacs.find(aofac);
			if(iter==aofacs.end()){
				vector<const JObject*> tmp;
				aofacs[aofac] = tmp;
			}
			// Record this object as belonging to this factory
			aofacs[aofac].push_back(aobjs[j]);
		}
		// Figure out number of spaces to indent objects based on factory name length
		map<JFactory_base*, vector<const JObject*> >::iterator iter;
		unsigned int indent=4; // some minimal string length
		for(iter=aofacs.begin(); iter!=aofacs.end(); iter++){
			JFactory_base *fac = iter->first;
			string name = fac->GetDataClassName();
			if(strlen(fac->Tag())!=0)name += string(":") + fac->Tag();
			if(name.length()>indent)indent=name.length();
		}
		indent += 4; // indent the factory name itself
		
		// Loop over factories that produced associated objects for this object and
		// list the objects it created
		for(iter=aofacs.begin(); iter!=aofacs.end(); iter++){
			JFactory_base *fac = iter->first;
			vector<const JObject*> &ptrs = iter->second;
			
			// Print out factory name
			string name = fac->GetDataClassName();
			if(strlen(fac->Tag())!=0)name += string(":") + fac->Tag();
			cout<<string(indent-name.length()-1,' ');
			cout<<name<<" ";
			
			// Loop over objects from this factory that are in the list
			for(unsigned int j=0; j<ptrs.size(); j++){
				if(j!=0)cout<<string(indent,' ');
				cout<<"0x"<<hex<<(unsigned long)ptrs[j]<<dec<<endl;
			}
		}
	}
	
}

