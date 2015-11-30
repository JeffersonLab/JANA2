// Author: David Lawrence  August 8, 2007
//
//

#include <math.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <set>
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
	suppress_unused_factories = true;
	try{
		app->GetJParameterManager()->SetDefaultParameter("RECORD_CALL_STACK", record_call_stack);
		if(app->GetJParameterManager()->Exists("FORCE_ALL_FACTORIES_ACTIVE")){
			app->GetJParameterManager()->GetParameter("FORCE_ALL_FACTORIES_ACTIVE", force_all_factories_active);
		}
		if(app->GetJParameterManager()->Exists("JANADOT:FOCUS")){
			has_focus = true;
			app->GetJParameterManager()->GetParameter("JANADOT:FOCUS", focus_factory);
			jout<<" Setting JANADOT focus to: "<<focus_factory<<endl;
		}
		app->GetJParameterManager()->SetDefaultParameter("JANADOT:SUPPRESS_UNUSED_FACTORIES", suppress_unused_factories, "If true, then do not list factories in groups that did not show up in list of factories recorded during processing. If false, these will show up as white ovals with no connections (ghosts)");

		// User can specify grouping using configuration parameters starting with
		// "JANADOT:GROUP:". The remainder of the parameter name is used to name
		// the group and the value is a comma separated list of classes to add to
		// the group.
		map<string,string> parms;
		app->GetJParameterManager()->GetParameters(parms, "JANADOT:GROUP:");
		jout<<"JANADOT groups: "<<parms.size()<<endl;
		for(map<string,string>::iterator it=parms.begin(); it!=parms.end(); it++){
		
			string group_name = it->first;
			jout<<" JANADOT group \""<<group_name<<"\" found ";
			
			// Split vals at commas
			vector<string> myclasses;
			string str = it->second;
			unsigned int cutAt;
			while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
				if(cutAt > 0){
					string val = str.substr(0,cutAt);
					
					// Check if a color was specified
					if(val.find("color_")==0){
						group_colors[group_name] = val.substr(6);
					}else if(val.find("no_box")==0){
						no_subgraph_groups.insert(group_name);
					}else{
						myclasses.push_back(val);
					}
				}
				str = str.substr(cutAt+1);
			}
			if(str.length() > 0)myclasses.push_back(str);
			if(myclasses.size() > 0) groups[it->first] = myclasses;
			
			if(group_colors.find(it->first)!=group_colors.end()){
				jout<<" ("<<group_colors[it->first]<<") ";
			}
			jout << myclasses.size() << " classes" << endl;
		}
		
		// Make an inverted list keyed by node name that has the group color as the value
		map<string,vector<string> >::iterator it = groups.begin();
		for(; it!=groups.end(); it++){
			if(group_colors.find(it->first) == group_colors.end()) continue;
			string color = group_colors[it->first];
			vector<string> &classes = it->second;
			for(unsigned int i=0; i<classes.size(); i++) node_colors[classes[i]] = color;
		}

	}catch(...){}

	// Initialize our mutex
	pthread_mutex_init(&mutex, NULL);
	
	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::brun(JEventLoop *loop, int32_t runnumber)
{
	// Nothing to do here
	
	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorJANADOT::evnt(JEventLoop *loop, uint64_t eventnumber)
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
				fcallstats2.Nfrom_cache++;
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

	// In order to get the total time we have to first get a list of 
	// the event processors (i.e. top-level callers). We can tell
	// this just by looking for callers that never show up as callees
	set<string> callers;
	set<string> callees;
	map<CallLink, CallStats>::iterator iter;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;
		string caller = MakeNametag(link.caller_name, link.caller_tag);
		string callee = MakeNametag(link.callee_name, link.callee_tag);
		callers.insert(caller);
		callees.insert(callee);
	}

	// Loop over list a second time so we can get the total ticks for
	// the process in order to add the percentage to the label below
	double total_ms = 0.0;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;
		CallStats &stats = iter->second;
		string caller = MakeNametag(link.caller_name, link.caller_tag);
		string callee = MakeNametag(link.callee_name, link.callee_tag);

		if(callees.find(caller) == callees.end()){
			total_ms += stats.from_factory_ms + stats.from_source_ms + stats.from_cache_ms + stats.data_not_available_ms;
		}
	}
	if(total_ms == 0.0)total_ms = 1.0;
	
	// If the user specified a focus factory, find the decendents and ancestors
	set<string> focus_relatives;
	if(has_focus){
		FindDecendents(focus_factory, focus_relatives);
		FindAncestors(focus_factory, focus_relatives);
	}

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
		
		// Don't draw links when the caller is flagged to be ignored via the special name "<ignore>"
		if(nametag1 == "<ignore>") continue;
		
		double my_ms = stats.from_factory_ms + stats.from_source_ms + stats.from_cache_ms;
		double percent = 100.0*my_ms/total_ms;
		char percentstr[32];
		sprintf(percentstr, "%5.1f%%", percent);
		
		string timestr=MakeTimeString(stats.from_factory_ms);
		
		// If a focus factory was specified, check if either the 
		if(has_focus){
			if(focus_relatives.find(nametag1)==focus_relatives.end()) continue;
			if(focus_relatives.find(nametag2)==focus_relatives.end()) continue;
		}
		
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
	vector<string> processor_nodes;
	set<string> factory_nodes;
	map<string, FactoryCallStats>::iterator fiter;
	for(fiter=factory_stats.begin(); fiter!=factory_stats.end(); fiter++){
		FactoryCallStats &fcall_stats = fiter->second;
		string nodename = fiter->first;
		
		// If nodename has special value "<ignore>" then do not draw it. This
		// is used by event sources that want to draw the low level objects that
		// were made, even though they weren't specifically requested. The 
		// caller name would then be set to this value and the tag an empty string.
		if(nodename == "<ignore>") continue;
		
		// If a focus was specified then check if this node is a relative or not
		if(has_focus){
			if(focus_relatives.find(nodename)==focus_relatives.end()) continue;
		}

		// Decide whether this is a factory, processor, or source
		if(fcall_stats.Nfrom_source==0 && fcall_stats.Nfrom_factory==0 && fcall_stats.Nfrom_cache==0){
			fcall_stats.type = kProcessor;
		}else if(fcall_stats.Nfrom_factory > 0){
			fcall_stats.type = kFactory;
		}else if(fcall_stats.Nfrom_cache>0 && fcall_stats.Nfrom_source==0){
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
				if(nodename == "AutoActivated"){
					fillcolor = "lightgrey";
					shape = "hexagon";
				}
				processor_nodes.push_back(fiter->first);
				break;
			case kFactory:
				fillcolor = "lightblue";
				if(node_colors.find(nodename)!=node_colors.end()) fillcolor = node_colors[nodename];
				shape = "box";
				if(has_focus && nodename==focus_factory) shape = "tripleoctagon";
				factory_nodes.insert(nodename);
				break;
			case kSource:
				fillcolor = "green";
				shape = "trapezium";
				source_nodes.push_back(nodename);
				break;
			case kDefault:
			default:
				fillcolor = "lightgrey";
				shape = "hexagon";
				break;
		}

		stringstream label_html;
		label_html<<"<TABLE border=\"0\" cellspacing=\"0\" cellpadding=\"0\" cellborder=\"0\">";
		label_html<<"<TR><TD>"<<nodename<<"</TD></TR>";
		if(fcall_stats.type!=kProcessor){
			label_html<<"<TR><TD><font point-size=\"8\">"<<timestr<<" ("<<percentstr<<")</font></TD></TR>";
		}
		label_html<<"</TABLE>";
		
		file<<"\t\""<<nodename<<"\"";
		file<<" [shape="<<shape<<",style=filled,fillcolor="<<fillcolor;
		file<<", label=<"<<label_html.str()<<">";
		if(fcall_stats.type==kSource)file<<", margin=0";
		file<<"];"<<endl;
	}
	
	// Make node to specify time
	time_t now = time(NULL);
	string datetime(ctime(&now));
	datetime.erase(datetime.length()-1);
	stringstream label_html;
	label_html << "<font point-size=\"10\">";
	label_html << "Created: "<<datetime;
	label_html << "</font>";
	file<<"\t\"CreationTime\"";
	file<<" [shape=box,style=filled,color=white";
	file<<", label=<"<<label_html.str()<<">";
	file<<", margin=0";
	file<<"];"<<endl;
	

	// Make all processor nodes appear at top of graph
	file<<"\t{rank=source; ";
	file << "\"CreationTime\";";
	for(unsigned int i=0; i<processor_nodes.size(); i++)file<<"\""<<processor_nodes[i]<<"\"; ";
	file<<"}"<<endl;
	
	// Make all source nodes appear on bottom of graph
	file<<"\t{rank=sink; ";
	for(unsigned int i=0; i<source_nodes.size(); i++)file<<"\""<<source_nodes[i]<<"\"; ";
	file<<"}"<<endl;
	
	// Add all groups as subgraphs
	int icluster=0;
	map<string,vector<string> >::iterator it=groups.begin();
	for(; it!=groups.end(); it++, icluster++){
		string group_name = it->first;
		if(no_subgraph_groups.find(group_name) != no_subgraph_groups.end()) continue;

		file << "subgraph cluster_" << icluster <<" {";
		vector<string> &myclasses = it->second;
		for(unsigned int i=0; i<myclasses.size(); i++){
			bool include_class = true;
			if(suppress_unused_factories){
				include_class = (factory_nodes.find(myclasses[i]) != factory_nodes.end());
			}
			if(include_class){
				file << "\"" << myclasses[i] << "\"; ";
			}
		}
		file << "label=\"" << group_name << "\"; ";
		
		string color = "forestgreen"; // default box color
		//if(group_colors.find(group_name)!=group_colors.end()) color = group_colors[group_name];
		file << "color="<<color<<"}" << endl;
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

//------------------------------------------------------------------
// FindDecendents
//------------------------------------------------------------------
void JEventProcessorJANADOT::FindDecendents(string caller, set<string> &decendents)
{
	/// The is reentrant and will call itself until all decendents of the
	/// given caller are recorded in decendents

	decendents.insert(caller);

	map<CallLink, CallStats>::iterator iter;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;

		string mycaller = MakeNametag(link.caller_name, link.caller_tag);
		string mycallee = MakeNametag(link.callee_name, link.callee_tag);
		
		if(mycaller == caller) FindDecendents(mycallee, decendents);
	}

}

//------------------------------------------------------------------
// FindAncestors
//------------------------------------------------------------------
void JEventProcessorJANADOT::FindAncestors(string callee, set<string> &ancestors)
{
	/// The is reentrant and will call itself until all decendents of the
	/// given caller are recorded in decendents

	ancestors.insert(callee);

	map<CallLink, CallStats>::iterator iter;
	for(iter=call_links.begin(); iter!=call_links.end(); iter++){
		const CallLink &link = iter->first;

		string mycaller = MakeNametag(link.caller_name, link.caller_tag);
		string mycallee = MakeNametag(link.callee_name, link.callee_tag);
		
		if(mycallee == callee) FindAncestors(mycaller, ancestors);
	}

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

