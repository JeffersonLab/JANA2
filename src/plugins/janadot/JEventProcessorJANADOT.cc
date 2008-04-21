// Author: David Lawrence  August 8, 2007
//
//

#include <iostream>
#include <fstream>
using namespace std;

#include <JANA/JApplication.h>
#include "JEventProcessorJANADOT.h"
using namespace jana;


// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new JEventProcessorJANADOT());
}
} // "C"

//------------------------------------------------------------------
// init 
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::init(void)
{
	// Turn on call stack recording
	bool record_call_stack=true;
	force_all_factories_active = false;
	app->GetJParameterManager()->SetDefaultParameter("RECORD_CALL_STACK", record_call_stack);
	app->GetJParameterManager()->GetParameter("FORCE_ALL_FACTORIES_ACTIVE", force_all_factories_active);
	
	// Initialize our mutex
	pthread_mutex_init(&mutex, NULL);
	
	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::brun(JEventLoop *loop, int runnumber)
{
	// Nothing to do here
	
	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::evnt(JEventLoop *loop, int eventnumber)
{
	// If we are supposed to activate all factories, do that now
	if(force_all_factories_active){
		vector<JFactory_base*> factories = loop->GetFactories();
		for(unsigned int i=0; i<factories.size(); i++)factories[i]->GetNrows();
	}

	// Get the call stack for ths event and add the results to our stats
	vector<JEventLoop::call_stack_t> stack = loop->GetCallStack();
	
	// Lock mutex in case we are running with multiple threads
	pthread_mutex_lock(&mutex);
	
	// Loop over the call stack elements and add in the values
	for(unsigned int i=0; i<stack.size(); i++){
		CallLink link;
		link.caller_name = stack[i].caller_name;
		link.caller_tag  = stack[i].caller_tag;
		link.callee_name = stack[i].callee_name;
		link.callee_tag  = stack[i].callee_tag;

		CallStats &stats = call_links[link]; // get pointer to stats object or create if it doesn't exist
		
		clock_t delta_t = stack[i].end_time - stack[i].start_time;
		switch(stack[i].data_source){
			case JEventLoop::DATA_NOT_AVAILABLE:
				stats.Ndata_not_available++;
				stats.data_not_available_ticks += delta_t;
				break;
			case JEventLoop::DATA_FROM_CACHE:
				stats.Nfrom_cache++;
				stats.from_cache_ticks += delta_t;
				break;
			case JEventLoop::DATA_FROM_SOURCE:
				stats.Nfrom_source++;
				stats.from_source_ticks += delta_t;
				break;
			case JEventLoop::DATA_FROM_FACTORY:
				stats.Nfrom_factory++;
				stats.from_factory_ticks += delta_t;
				break;				
		}
	}
	
	// Unlock mutex
	pthread_mutex_unlock(&mutex);

	return NOERROR;
}

//------------------------------------------------------------------
// fini
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::fini(void)
{
	// Open dot file for writing
	cout<<"Opening output file \"jana.dot\""<<endl;
	ofstream file("jana.dot");
	
	file<<"digraph G {"<<endl;

	// Loop over call links
	map<CallLink, CallStats>::iterator iter;
	map<string,bool> factory_names;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;
		CallStats &stats = iter->second;
		
		unsigned int Ntotal = stats.Nfrom_cache + stats.Nfrom_source + stats.Nfrom_factory;
		string nametag1 = link.caller_name;
		string nametag2 = link.callee_name;
		if(link.caller_tag.size()>0)nametag1 += ":"+link.caller_tag;
		if(link.callee_tag.size()>0)nametag2 += ":"+link.callee_tag;
		
		file<<"\t";
		file<<"\""<<nametag1<<"\"";
		file<<" -> ";
		file<<"\""<<nametag2<<"\"";
		file<<" [style=bold, fontsize=8, label=\""<<Ntotal<<" calls\\n"<<stats.from_factory_ticks<<" ticks\"];";
		file<<endl;
		
		// Keep a list of factory nametags so we can write out their
		// drawing attributes as separate dot commands just once at the end
		factory_names[nametag1] = true;
		factory_names[nametag2] = true;
	}
	
	file<<endl;

	// Add commands for node drawing style
	map<string, bool>::iterator fiter;
	for(fiter=factory_names.begin(); fiter!=factory_names.end(); fiter++){
		string fillcolor = "lightblue"; // default for factories
		string shape = "box"; // default shape for factories
		if(fiter->first == "DEventProcessor"){
			fillcolor="aquamarine";
			shape = "ellipse";
		}
		
		file<<"\t\""<<fiter->first<<"\"";
		file<<" [shape="<<shape<<",style=filled,fillcolor="<<fillcolor<<"];"<<endl;
	}
	
	// Close file
	file<<"}"<<endl;
	file.close();
	
	// Print message to tell user how to use the dot file
	cout<<endl
	<<"Factory calling information written to \"jana.dot\". To create a graphic"<<endl
	<<"from this, use the dot program. For example, to make a PDF file using an"<<endl
	<<"intermediate postscript file do the following:"<<endl
	<<endl
	<<"   dot -Tps2 jana.dot -o jana.ps"<<endl
	<<"   ps2pdf jana.ps"<<endl
	<<endl
	<<"This should give you a file named \"jana.pdf\"."<<endl
	<<endl;

	return NOERROR;
}

