// $Id$
//
//    File: JEventProcessor_janaview.cc
// Created: Fri Oct  3 08:14:14 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#include <unistd.h>
#include <set>
using namespace std;

#include "JEventProcessor_janaview.h"
using namespace jana;

jv_mainframe *JVMF = NULL;
JEventProcessor_janaview *JEP=NULL;


// Routine used to create our JEventProcessor
#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new JEventProcessor_janaview());
}
} // "C"

//==============================================================================


//.....................................
// FactoryNameSort
//.....................................
bool FactoryNameSort(JFactory_base *a,JFactory_base *b){
  if (a->GetDataClassName()==b->GetDataClassName()) return string(a->Tag()) < string(b->Tag());
  return a->GetDataClassName() < b->GetDataClassName();
}

//==============================================================================

//-------------------
// JanaViewRootGUIThread
//
// C-callable routine that can be used with pthread_create to launch
// a thread that creates the ROOT GUI and handles all ROOT GUI interactions
//-------------------
void* JanaViewRootGUIThread(void *arg)
{
	JEventProcessor_janaview *jproc = (JEventProcessor_janaview*)arg;

	// Create a ROOT TApplication object
	int narg = 0;
	TApplication app("JANA Viewer", &narg, NULL);
	app.SetReturnFromRun(true);
	JVMF = new jv_mainframe(gClient->GetRoot(), 600, 600, true);
	
	try{
		app.Run();
	}catch(std::exception &e){
		_DBG_ << "Exception: " << e.what() << endl;
	}

	return NULL;
}

//==============================================================================


//------------------
// JEventProcessor_janaview (Constructor)
//------------------
JEventProcessor_janaview::JEventProcessor_janaview()
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	pthread_t root_thread;
	pthread_create(&root_thread, NULL, JanaViewRootGUIThread, this);
	
	while(!JVMF) usleep(100000);

	JEP = this;
}

//------------------
// ~JEventProcessor_janaview (Destructor)
//------------------
JEventProcessor_janaview::~JEventProcessor_janaview()
{

}

//------------------
// init
//------------------
jerror_t JEventProcessor_janaview::init(void)
{

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_janaview::brun(JEventLoop *eventLoop, int runnumber)
{
	eventLoop->EnableCallStackRecording();

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_janaview::evnt(JEventLoop *loop, int eventnumber)
{
	// We need to wait here in order to allow the GUI to control when to
	// go to the next event. Lock the mutex and wait for the GUI to wake us
	japp->monitor_heartbeat = false;
	japp->SetShowTicker(false);

	pthread_mutex_lock(&mutex);

	static bool processed_first_event = false;

	this->loop = loop;
	this->eventnumber = eventnumber;
	
	JEvent &jevent = loop->GetJEvent();
	JEventSource *source = jevent.GetJEventSource();
	JVMF->UpdateInfo(source->GetSourceName(), jevent.GetRunNumber(), jevent.GetEventNumber());

	vector<JVFactoryInfo> facinfo;
	GetObjectTypes(facinfo);
	JVMF->UpdateObjectTypeList(facinfo);

	pthread_cond_wait(&cond, &mutex);
	
	processed_first_event = true;
	
	pthread_mutex_unlock(&mutex);

	return NOERROR;
}

//------------------
// NextEvent
//------------------
void JEventProcessor_janaview::NextEvent(void)
{
	// This just unblocks the pthread_cond_wait() call in evnt(). 
	pthread_cond_signal(&cond);
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_janaview::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_janaview::fini(void)
{
	// Called before program exit after event processing is finished.
	return NOERROR;
}

//------------------------------------------------------------------
// MakeNametag
//------------------------------------------------------------------
string JEventProcessor_janaview::MakeNametag(const string &name, const string &tag)
{
	string nametag = name;
	if(tag.size()>0)nametag += ":"+tag;

	return nametag;
}

//------------------
// GetObjectTypes
//------------------
void JEventProcessor_janaview::GetObjectTypes(vector<JVFactoryInfo> &facinfo)
{
	// Get factory pointers and sort them by factory name
	vector<JFactory_base*> factories = loop->GetFactories();
	sort(factories.begin(), factories.end(), FactoryNameSort);
	
	// Copy factory info into JVFactoryInfo structures
	for(uint32_t i=0; i<factories.size(); i++){
		JVFactoryInfo finfo;
		finfo.name = factories[i]->GetDataClassName();
		finfo.tag = factories[i]->Tag();
		finfo.nametag = MakeNametag(finfo.name, finfo.tag);
		facinfo.push_back(finfo);
	}
}

//------------------
// GetAssociatedTo
//------------------
void JEventProcessor_janaview::GetAssociatedTo(JObject *jobj, vector<const JObject*> &associatedTo)
{
	vector<JFactory_base*> factories = loop->GetFactories();
	for(uint32_t i=0; i<factories.size(); i++){
		
		// Do not activate factories that have not yet been activated
		if(!factories[i]->evnt_was_called()) continue;
		
		// Get objects for this factory and associated objects for each of those
		vector<void*> vobjs = factories[i]->Get();
		for(uint32_t i=0; i<vobjs.size(); i++){
			JObject *obj = (JObject*)vobjs[i];
			
			vector<const JObject*> associated;
			obj->GetT(associated);
			
			for(uint32_t j=0; j<associated.size(); j++){
				if(associated[j] == jobj) associatedTo.push_back(obj);
			}
		}
	}
}

////------------------
//// FindAncestors
////------------------
//void JEventProcessor_janaview::FindAncestors(string nametag, set<string> &callers)
//{
//
//
//}

//------------------
// MakeCallGraph
//------------------
void JEventProcessor_janaview::MakeCallGraph(string nametag)
{
//	vector<JEventLoop::call_stack_t> stack = loop->GetCallStack();
//	
//	// Get ancestors
//	set<string> all_factories;
//	vector<set<string> > ancestor_chain;
//	while(true);
//		set<string> ancestors;
//		for(uint32_t i=0; i<stack.size(); i++){
//			JEventLoop::call_stack_t &cs = stack[i];
//			string caller = MakeNametag(cs.caller_name, cs.caller_tag);
//			string callee = MakeNametag(cs.callee_name, cs.callee_tag);
//			if(callee == nametag){
//				if(all_factories.find(caller) != all_factories.end()) continue;
//				ancestors.insert(caller);
//				all_factories.insert(caller);
//			}
//		}
//		if(ancestors.empty()) break;
//		
//		ancestor_chain.push_back(ancestors);
//	}
//
//
//
//
//	for(unsigned int i=0; i<stack.size(); i++){
//
//		// Keep track of total time each factory spent waiting and being waited on
//		string nametag1 = MakeNametag(stack[i].caller_name, stack[i].caller_tag);
//		string nametag2 = MakeNametag(stack[i].callee_name, stack[i].callee_tag);
//
//		FactoryCallStats &fcallstats1 = factory_stats[nametag1];
//		FactoryCallStats &fcallstats2 = factory_stats[nametag2];
//		
//		double delta_t = (stack[i].start_time - stack[i].end_time)*1000.0;
//		fcallstats1.time_waiting += delta_t;
//		fcallstats2.time_waited_on += delta_t;
//
//		// Get pointer to CallStats object representing this calling pair
//		CallLink link;
//		link.caller_name = stack[i].caller_name;
//		link.caller_tag  = stack[i].caller_tag;
//		link.callee_name = stack[i].callee_name;
//		link.callee_tag  = stack[i].callee_tag;
//		CallStats &stats = call_links[link]; // get pointer to stats object or create if it doesn't exist
//		
//		switch(stack[i].data_source){
//			case JEventLoop::DATA_NOT_AVAILABLE:
//				stats.Ndata_not_available++;
//				stats.data_not_available_ms += delta_t;
//				break;
//			case JEventLoop::DATA_FROM_CACHE:
//				fcallstats2.Nfrom_cache++;
//				stats.Nfrom_cache++;
//				stats.from_cache_ms += delta_t;
//				break;
//			case JEventLoop::DATA_FROM_SOURCE:
//				fcallstats2.Nfrom_source++;
//				stats.Nfrom_source++;
//				stats.from_source_ms += delta_t;
//				break;
//			case JEventLoop::DATA_FROM_FACTORY:
//				fcallstats2.Nfrom_factory++;
//				stats.Nfrom_factory++;
//				stats.from_factory_ms += delta_t;
//				break;				
//		}
//		
//	}
//	
//	set<string> focus_relatives;
//	FindDecendents(focus_factory, focus_relatives);
//	FindAncestors(focus_factory, focus_relatives);
//
}

