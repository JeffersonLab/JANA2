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

#include <TLatex.h>
#include <TArrow.h>
#include <TBox.h>

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
//	JEventProcessor_janaview *jproc = (JEventProcessor_janaview*)arg;

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
jerror_t JEventProcessor_janaview::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	eventLoop->EnableCallStackRecording();

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_janaview::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	// We need to wait here in order to allow the GUI to control when to
	// go to the next event. Lock the mutex and wait for the GUI to wake us
	japp->monitor_heartbeat = false;
	japp->SetShowTicker(false);

	pthread_mutex_lock(&mutex);

	// static bool processed_first_event = false;

	this->loop = loop;
	this->eventnumber = eventnumber;
	
	JEvent &jevent = loop->GetJEvent();
	JEventSource *source = jevent.GetJEventSource();
	JVMF->UpdateInfo(source->GetSourceName(), jevent.GetRunNumber(), jevent.GetEventNumber());

	vector<JVFactoryInfo> facinfo;
	GetObjectTypes(facinfo);
	JVMF->UpdateObjectTypeList(facinfo);
	
	MakeCallGraph();

	pthread_cond_wait(&cond, &mutex);
	
	// processed_first_event = true;
	
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

//------------------
// MakeCallGraph
//------------------
void JEventProcessor_janaview::MakeCallGraph(string nametag)
{

	// Clear canvas
	TCanvas *c = JVMF->canvas->GetCanvas();
	c->cd();
	c->Clear();
	c->Update();

	// Make list of all factories and their callees
	vector<JEventLoop::call_stack_t> stack = loop->GetCallStack();
	if(stack.empty()) return;
	map<string, CGobj*> cgobjs;
	for(uint32_t i=0; i<stack.size(); i++){
		JEventLoop::call_stack_t &cs = stack[i];
		string caller = MakeNametag(cs.caller_name, cs.caller_tag);
		string callee = MakeNametag(cs.callee_name, cs.callee_tag);
		if(caller == "<ignore>") continue;
		
		CGobj *caller_obj = cgobjs[caller];
		CGobj *callee_obj = cgobjs[callee];
		if( caller_obj==NULL ) caller_obj = cgobjs[caller] = new CGobj(caller);
		if( callee_obj==NULL ) callee_obj = cgobjs[callee] = new CGobj(callee);
		
		caller_obj->callees.insert(callee_obj);
	}

	// Continually loop, promoting all callers to be higher in rank than all
	// of their callees
	do{
		bool nothing_changed = true;
		map<string, CGobj*>::iterator iter = cgobjs.begin();

		for(; iter != cgobjs.end(); iter++){
		
			CGobj *cgobj = iter->second;
		
			set<CGobj*>::iterator it = cgobj->callees.begin();
			for(; it!=cgobj->callees.end(); it++){
				if(*it == cgobj) break; // in case we are listed as our own callee!
				if((*it)->rank >= cgobj->rank){
					cgobj->rank = (*it)->rank + 1;
					nothing_changed = false;
				}
			}

		}
		if(nothing_changed) break;
	}while(true);

	// Determine overall rank properties (widths, heights, members)
	map<Int_t, CGrankprop > rankprops;
	map<string, CGobj*>::iterator iter;
	for(iter=cgobjs.begin(); iter != cgobjs.end(); iter++){
		CGobj *cgobj = iter->second;
		CGrankprop &rankprop = rankprops[cgobj->rank];
		
		rankprop.cgobjs.push_back(cgobj);
		rankprop.totheight += cgobj->h;
		if((int32_t)cgobj->w > rankprop.totwidth) rankprop.totwidth = cgobj->w;
	}
	
	// Get minimum width and height of canvas needed to display everything
	Int_t totwidth  = 0; // left edge gap in pixels
	Int_t totheight = 0;
	Int_t Nx = rankprops.size(); // number of columns (ranks)
	Int_t Ny = 0; // number of rows in tallest rank
	map<Int_t, CGrankprop >::iterator itrp;
	for(itrp=rankprops.begin(); itrp!=rankprops.end(); itrp++){
	
		CGrankprop &rprop = itrp->second;
		totwidth += rprop.totwidth;
		Int_t height = rprop.totheight;
		if(totheight < height){
			totheight = height;
			Ny = rprop.cgobjs.size();
		}
	}
	Int_t xspace = 50; // minimum number of pixels between columns
	Int_t yspace = 10; // minimum number of pixels between rows
	Int_t minwidth  = totwidth  + (Nx+1)*xspace;
	Int_t minheight = totheight + (Ny+1)*yspace;
	
	// Get actual height and width of canvas so we can either make it
	// bigger or increase our spacing.
	// Note that I spent a LOT of time trying to get this to work right.
	// In the end, I never could. I'm going to have punt now and call it 
	// "somewhat usable".
	Int_t cwidth  = JVMF->fTab->GetWidth();
	Int_t cheight = JVMF->fTab->GetHeight();
	if( cwidth  < minwidth  ) cwidth  = minwidth;
	if( cheight < minheight ) cheight = minheight;
	
	// Loop over all ranks again, setting x and y spacing and
	// using them to calculate the box coordinates for each factory
	xspace = (cwidth - totwidth)/(Nx+1);
	Int_t xpos = xspace/2;
	for(itrp=rankprops.begin(); itrp!=rankprops.end(); itrp++){
	
		CGrankprop &rprop = itrp->second;
		yspace = (cheight - rprop.totheight)/(rprop.cgobjs.size()+1);
		Int_t ypos = yspace;
		for(uint32_t i=0; i<rprop.cgobjs.size(); i++){
			CGobj *cgobj = rprop.cgobjs[i];
			
			Int_t pad = 4;
			cgobj->x1 = xpos - pad;
			cgobj->x2 = xpos + cgobj->w + pad;
			cgobj->y1 = ypos - pad;
			cgobj->y2 = ypos + cgobj->h + pad;
			cgobj->ymid = (cgobj->y2 + cgobj->y1)/2;
			
			ypos += cgobj->h + yspace;
		}

		xpos += rprop.totwidth + xspace;
	}

	// Clear canvas
	JVMF->canvas->SetWidth(  cwidth  );
	JVMF->canvas->SetHeight( cheight );
	c->SetCanvasSize( cwidth, cheight );
	c->SetMargin(0.0, 0.0, 0.0, 0.0);
	c->Clear();
	c->Update();
	
	// Draw links first
	for(iter=cgobjs.begin(); iter != cgobjs.end(); iter++){
		CGobj *cgobj1 = iter->second;
		set<CGobj*> &callees = cgobj1->callees;
		set<CGobj*>::iterator it = callees.begin();
		double x1 = cgobj1->x1/(double)cwidth;
		double y1 = cgobj1->ymid/(double)cheight;
		for(; it!=callees.end(); it++){
			CGobj *cgobj2 = *it;
			double x2 = cgobj2->x2/(double)cwidth;
			double y2 = cgobj2->ymid/(double)cheight;
			
			TLine *lin = new TLine(x1,y1,x2,y2);
			lin->SetLineColor(kBlack);
			lin->SetLineWidth(1.0);
			lin->Draw();
		}
	}

	// Draw boxes with factory names
	TLatex latex;
	latex.SetTextSizePixels(20);
	latex.SetTextAlign(22);
	latex.SetTextColor(kWhite);
	for(iter=cgobjs.begin(); iter != cgobjs.end(); iter++){
		CGobj *cgobj = iter->second;
		double x1 = cgobj->x1/(double)cwidth;
		double x2 = cgobj->x2/(double)cwidth;
		double y1 = cgobj->y1/(double)cheight;
		double y2 = cgobj->y2/(double)cheight;

		TBox *box = new TBox(x1, y1, x2, y2);
		box->SetFillColor(TColor::GetColor( (Float_t)0.4, 0.4, 0.4));
		box->Draw();
		latex.DrawLatex((x1+x2)/2.0, (y1+y2)/2.0, cgobj->nametag.c_str());
	}
	
	TBox *box = new TBox(0.0, 0.0, 1.0, 1.0);
	box->SetFillStyle(0);
	box->SetLineWidth(4);
	box->SetLineColor(kRed);
	box->Draw();

	c->Update();
}

