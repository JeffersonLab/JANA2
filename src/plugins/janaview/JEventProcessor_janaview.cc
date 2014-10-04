// $Id$
//
//    File: JEventProcessor_janaview.cc
// Created: Fri Oct  3 08:14:14 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#include <unistd.h>

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
	// This is called once at program startup. If you are creating
	// and filling historgrams in this plugin, you should lock the
	// ROOT mutex like this:
	//
	// japp->RootWriteLock();
	//  ... fill historgrams or trees ...
	// japp->RootUnLock();
	//

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_janaview::brun(JEventLoop *eventLoop, int runnumber)
{
	// This is called whenever the run number changes
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
		finfo.nametag = finfo.name;
		if(finfo.tag != "") finfo.nametag += ":" + finfo.tag;
		facinfo.push_back(finfo);
	}
}

