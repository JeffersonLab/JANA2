// Author: David Lawrence  August 8, 2007
//
//

#include <math.h>

#include <iostream>
#include <iomanip>
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
	try{
		app->GetJParameterManager()->SetDefaultParameter("RECORD_CALL_STACK", record_call_stack);
		app->GetJParameterManager()->GetParameter("FORCE_ALL_FACTORIES_ACTIVE", force_all_factories_active);
	}catch(...){}

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

		// Keep track of total time each factory spent waiting and being waited on
		string nametag1 = MakeNametag(stack[i].caller_name, stack[i].caller_tag);
		string nametag2 = MakeNametag(stack[i].callee_name, stack[i].callee_tag);

		FactoryCallStats &fcallstats1 = factory_stats[nametag1];
		FactoryCallStats &fcallstats2 = factory_stats[nametag2];
		
		double delta_t = (stack[i].start_time - stack[i].end_time)*1000.0;
		fcallstats1.time_waiting += delta_t;
		fcallstats2.time_waited_on += delta_t;

		// Get pointer to CallStats object representing this calling pair
		CallLink link;
		link.caller_name = stack[i].caller_name;
		link.caller_tag  = stack[i].caller_tag;
		link.callee_name = stack[i].callee_name;
		link.callee_tag  = stack[i].callee_tag;

		CallStats &stats = call_links[link]; // get pointer to stats object or create if it doesn't exist
		
		switch(stack[i].data_source){
			case JEventLoop::DATA_NOT_AVAILABLE:
				stats.Ndata_not_available++;
				stats.data_not_available_ms += delta_t;
				break;
			case JEventLoop::DATA_FROM_CACHE:
				stats.Nfrom_cache++;
				stats.from_cache_ms += delta_t;
				break;
			case JEventLoop::DATA_FROM_SOURCE:
				fcallstats2.Nfrom_source++;
				stats.Nfrom_source++;
				stats.from_source_ms += delta_t;
				break;
			case JEventLoop::DATA_FROM_FACTORY:
				fcallstats2.Nfrom_factory++;
				stats.Nfrom_factory++;
				stats.from_factory_ms += delta_t;
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
	// Find the JEventProcessor link so we can get the total ticks for
	// the process in order to add the percentage to the label below
	map<CallLink, CallStats>::iterator iter;
	double total_ms = 0.0;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;
		CallStats &stats = iter->second;
		if(link.caller_name=="JEventProcessor"){
			total_ms += stats.from_factory_ms + stats.from_source_ms + stats.from_cache_ms + stats.data_not_available_ms;
		}
	}
	if(total_ms == 0.0)total_ms = 1.0;

	// Open dot file for writing
	cout<<"Opening output file \"jana.dot\""<<endl;
	ofstream file("jana.dot");
	
	file<<"digraph G {"<<endl;

	// Loop over call links
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;
		CallStats &stats = iter->second;
		
		unsigned int Ntotal = stats.Nfrom_cache + stats.Nfrom_source + stats.Nfrom_factory;
		string nametag1 = MakeNametag(link.caller_name, link.caller_tag);
		string nametag2 = MakeNametag(link.callee_name, link.callee_tag);
		double my_ms = stats.from_factory_ms + stats.from_source_ms + stats.from_cache_ms;
		double percent = 100.0*my_ms/total_ms;
		char percentstr[32];
		sprintf(percentstr, "%5.1f%%", percent);
		
		string timestr=MakeTimeString(stats.from_factory_ms);
		
		file<<"\t";
		file<<"\""<<nametag1<<"\"";
		file<<" -> ";
		file<<"\""<<nametag2<<"\"";
		file<<" [style=bold, fontsize=8";
		file<<", label=\""<<Ntotal<<" calls\\n"<<timestr<<"\\n"<<percentstr<<"\"";
		//file<<", penwidth="<<(int)(percent/10.0);
		file<<"];";
		file<<endl;
	}
	
	file<<endl;

	// Add commands for node drawing style
	vector<string> source_nodes;
	map<string, FactoryCallStats>::iterator fiter;
	for(fiter=factory_stats.begin(); fiter!=factory_stats.end(); fiter++){
		FactoryCallStats &fcall_stats = fiter->second;

		// Decide whether this is a factory, processor, or source
		if(fcall_stats.Nfrom_source==0 && fcall_stats.Nfrom_factory==0){
			fcall_stats.type = kProcessor;
		}else if(fcall_stats.Nfrom_source < fcall_stats.Nfrom_factory){
			fcall_stats.type = kFactory;
		}else{
			fcall_stats.type = kSource;
		}
		
		// Get time spent in this factory proper
		double time_spent_in_factory = fcall_stats.time_waited_on - fcall_stats.time_waiting;
		string timestr=MakeTimeString(time_spent_in_factory);

		double percent = 100.0*time_spent_in_factory/total_ms;
		char percentstr[32];
		sprintf(percentstr, "%5.1f%%", percent);
		
		string fillcolor;
		string shape;
		switch(fcall_stats.type){
			case kProcessor:
				fillcolor = "aquamarine";
				shape = "ellipse";
				break;
			case kFactory:
				fillcolor = "lightblue";
				shape = "box";
				break;
			case kSource:
				fillcolor = "green";
				shape = "trapezium";
				source_nodes.push_back(fiter->first);
				break;
			case kDefault:
			default:
				fillcolor = "lightgrey";
				shape = "hexagon";
				break;
		}

		stringstream label_html;
		label_html<<"<TABLE border=\"0\" cellspacing=\"0\" cellpadding=\"0\" cellborder=\"0\">";
		label_html<<"<TR><TD>"<<fiter->first<<"</TD></TR>";
		if(fcall_stats.type!=kProcessor){
			label_html<<"<TR><TD><font point-size=\"8\">"<<timestr<<" ("<<percentstr<<")</font></TD></TR>";
		}
		label_html<<"</TABLE>";
		
		file<<"\t\""<<fiter->first<<"\"";
		file<<" [shape="<<shape<<",style=filled,fillcolor="<<fillcolor;
		file<<", label=<"<<label_html.str()<<">";
		if(fcall_stats.type==kSource)file<<", margin=0.,0.";
		file<<"];"<<endl;
	}
	
	// Make all source nodes appear on bottom of graph
	file<<"\t{rank=sink; ";
	for(unsigned int i=0; i<source_nodes.size(); i++)file<<"\""<<source_nodes[i]<<"\"; ";
	file<<"}"<<endl;
	
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

//------------------------------------------------------------------
// MakeTimeString
//------------------------------------------------------------------
string JEventProcessorJANADOT::MakeTimeString(double time_in_ms)
{
	double order=log10(time_in_ms);
	stringstream ss;
	ss<<fixed<<setprecision(2);
	if(order<0){
		ss<<time_in_ms*1000.0<<" us";
	}else if(order<=2.0){
		ss<<time_in_ms<<" ms";
	}else{
		ss<<time_in_ms/1000.0<<" s";
	}
	
	return ss.str();
}

//------------------------------------------------------------------
// MakeNametag
//------------------------------------------------------------------
string JEventProcessorJANADOT::MakeNametag(const string &name, const string &tag)
{
		string nametag = name;
		if(tag.size()>0)nametag += ":"+tag;
		
		return nametag;
}

