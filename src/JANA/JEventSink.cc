// $Id: JEventSink.cc 1433 2005-12-22 14:39:08Z davidl $
//
//    File: JEventSink.cc
// Created: Mon Dec 19 16:15:49 EST 2005
// Creator: davidl (on Linux phecda 2.6.9-11.ELsmp athlon)
//


#include "JEventSink.h"
#include "JEventLoop.h"
using namespace std;
using namespace jana;

//---------------------------------
// JEventSink    (Constructor)
//---------------------------------
JEventSink::JEventSink()
{
	initialized = false;
	pthread_mutex_init(&sink_mutex, NULL);
}

//---------------------------------
// brun
//---------------------------------
jerror_t JEventSink::brun(JEventLoop *loop, int32_t runnumber)
{
	jerror_t err=NOERROR;

	// We want to make sure that the factory write list is
	// generated only once.
	if(initialized)return NOERROR;
	LockSink();
	if(!initialized){
	
		err = brun_sink(loop, runnumber);
		initialized = true;
	}
	UnlockSink();

	return err;
}

//---------------------------------
// AddToWriteList
//---------------------------------
void JEventSink::AddToWriteList(string name, string tag)
{
	// We don't want to add a factory to the list twice!
	if(IsInWriteList(name,tag))return;

	factory_name_spec_t f;
	f.name = name;
	f.tag = tag;
	factories_to_write.push_back(f);
}

//---------------------------------
// AddAllToWriteList
//---------------------------------
void JEventSink::AddAllToWriteList(JEventLoop *loop)
{
	// Get list of all factories
	vector<JFactory_base*> factories = loop->GetFactories();
	for(unsigned int i=0; i<factories.size(); i++){
		if(!factories[i]->TestFactoryFlag(JFactory_base::WRITE_TO_OUTPUT))continue;
		AddToWriteList(factories[i]->GetDataClassName(), factories[i]->Tag());
	}
}

//---------------------------------
// RemoveFromWriteList
//---------------------------------
void JEventSink::RemoveFromWriteList(string name, string tag)
{
	vector<factory_name_spec_t>::iterator iter = factories_to_write.begin();
	for(; iter != factories_to_write.end(); iter++){
		if((*iter).name == name){
			if((*iter).tag == tag){
				factories_to_write.erase(iter);
				return;
			}
		}
	}
}

//---------------------------------
// PrintWriteList
//---------------------------------
void JEventSink::PrintWriteList(void)
{
	jout<<endl;
	jout<<"Factories to written to file (format is factory:tag)"<<endl;
	jout<<"----------------------------------------------------"<<endl;
	vector<factory_name_spec_t>::iterator iter = factories_to_write.begin();
	for(; iter != factories_to_write.end(); iter++){
		jout<<(*iter).name;
		if((*iter).tag.size()>0)jout<<":"<<(*iter).tag;
		jout<<endl;
	}
	jout<<endl;
}

//---------------------------------
// ClearWriteList
//---------------------------------
void JEventSink::ClearWriteList(void)
{
	factories_to_write.clear();
}

