// $Id$
//
//    File: JEventProcessor_janahbook.cc
// Created: Mon Aug 11 09:00:05 EDT 2008
// Creator: davidl (on Darwin Amelia.local 8.11.1 i386)
//

#include <iostream>
#include <fstream>
using namespace std;

#include <JANA/JApplication.h>
#include "JEventProcessor_janahbook.h"
using namespace jana;

#include "cern_c.h"
#define MEMH 8000000
#define LREC 1024		/* record length of hbook direct access file in WORDS */
#define LUN 3			/* logical unit number of hbook file */
extern "C" {
	float pawc_[MEMH];
	int quest_[100];
};

typedef JEventProcessor_janahbook::TreeInfo TreeInfo;

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new JEventProcessor_janahbook());
}
} // "C"

// Tokenize a string
static inline void Tokenize(string str, vector<string> &tokens, const char delim=' ')
{
	tokens.clear();
	unsigned int cutAt;
	while( (cutAt = str.find(delim)) != (unsigned int)str.npos ){
		if(cutAt > 0)tokens.push_back(str.substr(0,cutAt));
		str = str.substr(cutAt+1);
	}
	if(str.length() > 0)tokens.push_back(str);
}

//-----------------------------------------
// JEventProcessor_janahbook (constructor)
//-----------------------------------------
JEventProcessor_janahbook::JEventProcessor_janahbook()
{
	// Initialize mutex
	pthread_mutex_init(&rootmutex, NULL);
	pthread_mutex_lock(&rootmutex);
	
	// Initialize cernlib (monkey shines!)
	quest_[9] = 65000;
	int memh = MEMH;
	hlimit(memh);
	
	// Open HBOOK file for writing
	hropen(LUN, "lun", "janahbook.hbook" , "N", LREC, 0);

	// Set maximum number of objects a factory can store in an event.
	// Note that this determines the size of the block in memory needed
	// for the the event so be conservative!
	Nmax = 200;

	// Initialize event counter
	Nevents = 0;

	// Release ROOT mutex
	pthread_mutex_unlock(&rootmutex);
}

//------------------
// init
//------------------
jerror_t JEventProcessor_janahbook::init(void)
{
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_janahbook::brun(JEventLoop *eventLoop, int runnumber)
{

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_janahbook::evnt(JEventLoop *loop, int eventnumber)
{
	// We assume that all factories we want to record have already been
	// activated and so contain the objects (note that the toStrings()
	// method won't create them if they don't exist). Thus, we lock
	// the rootmutex to prevent other threads from accessing the TreeInfo
	// buffers and writing to the ROOT file while we do. Note that
	// this does not prevent other threads from accessing the ROOT
	// globals and mucking things up for us there.
	pthread_mutex_lock(&rootmutex);

	// Get list of all foctories for this JEventLoop that have their
	// WRITE_TO_OUTPUT flag set.
	vector<JFactory_base*> allfactories = loop->GetFactories();
	vector<JFactory_base*> facs;
	for(unsigned int i=0; i<allfactories.size(); i++){
		if(allfactories[i]->TestFactoryFlag(JFactory_base::WRITE_TO_OUTPUT))facs.push_back(allfactories[i]);
	}
	
	// Clear all trees' "N" values for this event
	map<std::string, TreeInfo*>::iterator iter=trees.begin();
	for(; iter!=trees.end(); iter++)*(iter->second->Nptr) = 0;
	
	// Find ( or create) the tree and copy the objects into its buffers
	for(unsigned int i=0; i<facs.size(); i++){

		//=== TEMPORARY====
		//facs[i]->GetNrows();
		//=== TEMPORARY====

		TreeInfo *tinfo  = GetTreeInfo(facs[i]);
		if(!tinfo)continue;		
		
		FillTree(facs[i], tinfo);
	}
	
	// Fill all trees
	//for(iter=trees.begin(); iter!=trees.end(); iter++)iter->second->tree->Fill();
	
	Nevents++;
	
	// Release the ROOT mutex lock
	pthread_mutex_unlock(&rootmutex);
	
	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_janahbook::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_janahbook::fini(void)
{
	pthread_mutex_lock(&rootmutex);

	// Close hbook file
	int icycle=0;
	hrout(0,icycle,"T");
	hrend("lun");

	pthread_mutex_unlock(&rootmutex);

	return NOERROR;
}

//------------------
// GetTreeInfo
//------------------
TreeInfo* JEventProcessor_janahbook::GetTreeInfo(JFactory_base *fac)
{
	// Look for entry in our list of existing branches for this one
	string key = string(fac->GetDataClassName())+":"+fac->Tag();
	map<string, TreeInfo*>::iterator iter = trees.find(key);
	if(iter!=trees.end())return iter->second;
	
	// No ntuple currently exists. Try creating one.
	vector<vector<pair<string,string> > > items;
	fac->toStrings(items, true);
	
	// If no objects currently exists for this factory (either because
	// it was activated, but produced zero objects or was never even
	// activated) then we have no information by which to create a branch.
	// In this case, just return NULL now.
	if(items.size()==0)return NULL;
	
	// Create TreeInfo structure
	TreeInfo *tinfo = new TreeInfo;
	
	// Find next available Ntuple number
	tinfo->ntp_num=10;
	for(iter = trees.begin(); iter!=trees.end(); iter++){
		if(iter->second->ntp_num >= tinfo->ntp_num)tinfo->ntp_num = iter->second->ntp_num+1;
	}
	
	// Create Ntuple
	string tname = fac->GetDataClassName();
	if(strlen(fac->Tag())>0)tname += string("_") + fac->Tag();
	hbnt(tinfo->ntp_num, (char*)tname.c_str(), (char*)"Autogenerated from JANA objects");

	// Build up Ntuple definition string starting with "N"
	char str[256];
	sprintf(str,"N[0,%d]:I", Nmax);
	string ntp_def = str;
		
	// Loop over data members and extract their name and type to form branches
	tinfo->obj_size=0;
	for(unsigned int i=0; i<items[0].size(); i++){
		vector<string> tokens;
		Tokenize(items[0][i].first, tokens, ':');
		if(tokens.size()<3)continue;
		
		// If units are appended using a form like "px(cm)", then chop
		// off the units part and just use the "px".
		unsigned int cutAt = tokens[0].find('(');
		string iname = (cutAt==(unsigned int)tokens[0].npos) ? tokens[0]:tokens[0].substr(0,cutAt);
		
		// Sometimes the name can have special characters (spaces or "."s)
		// that aren't good for leaf names in branches
		for(size_t k=0; k<iname.size(); k++){
			if(iname[k]==' ')iname[k] = '_';
			if(iname[k]=='.')iname[k] = '_';
		}
		
		unsigned long item_size=0;
		data_type_t type = type_unknown;
		string branch_def;
		if(tokens[1]=="int"){
			branch_def = iname + "(N):I";
			item_size = sizeof(int);
			type = type_int;
		}else if(tokens[1]=="uint"){
			branch_def = iname + "(N):I";
			item_size = sizeof(unsigned int);
			type = type_uint;
		}else if(tokens[1]=="long"){
			branch_def = iname + "(N):I";
			item_size = sizeof(long);
			type = type_long;
		}else if(tokens[1]=="ulong"){
			branch_def = iname + "(N):I";
			item_size = sizeof(unsigned long);
			type = type_ulong;
		}else if(tokens[1]=="float"){
			branch_def = iname + "(N):R";
			item_size = sizeof(float);
			type = type_float;
		}else if(tokens[1]=="double"){
			branch_def = iname + "(N):R";
			item_size = sizeof(double);
			type = type_double;
		}else if(tokens[1]=="string"){
			branch_def = iname + "(N):I";
			item_size = 256; // not sure what to do here!
			type = type_string;
		}else{
			branch_def = iname + "(N):R";
			item_size = sizeof(float);
			type = type_unknown;
		}
		
		// Record the address offest and increase object size
		tinfo->types.push_back(type);
		tinfo->item_sizes.push_back(item_size);
		tinfo->obj_size += item_size;
		
		// Add to Ntuple definition
		ntp_def += "," + branch_def;
	}
	
	// Inform user if nothing useful in the object is found
	if(tinfo->item_sizes.size()<1){
		_DBG_<<"No usable data members in \""<<tname<<"\"!"<<endl;
	}
	
	// Fill in TreeInfo object
	tinfo->Nmax = Nmax;
	tinfo->buff_size = sizeof(int) + tinfo->Nmax*tinfo->obj_size;
	tinfo->buff = new char[tinfo->buff_size];
	tinfo->Nptr = (int*)tinfo->buff;
	tinfo->Bptr = (unsigned long)&tinfo->Nptr[1];

	// Define Ntuple
	cout<<ntp_def<<endl;
	hbname(10,"EVNT", tinfo->buff, (char*)ntp_def.c_str());

	// Add to list
	trees[key] = tinfo;
	
	// At this point we may have already seen any number of events and other classes
	// may have already been added. We want to add Nevents of "NULL" events to the
	// current tree so they will be aligned and we can make them "friends".
	*tinfo->Nptr = 0;
	//for(unsigned long i=0; i<Nevents; i++)tinfo->tree->Fill();
	
	tinfo->Print();
	
	return tinfo;
}

//------------------
// FillTree
//------------------
void JEventProcessor_janahbook::FillTree(JFactory_base *fac, TreeInfo *tinfo)
{
#if 0
	// Get data in form of strings from factory
	vector<vector<pair<string,string> > > items;
	fac->toStrings(items, true);
	
	// Verify each object has the right number of elements
	unsigned int Nitems = tinfo->types.size(); // Number of items used to define object in tree
	for(unsigned int i=0; i<items.size(); i++){
		if(items[i].size()!=Nitems){
			_DBG_<<"Number of items inconsistent for Tree "<<tinfo->tree->GetName()
			<<" Nitems="<<Nitems<<" items["<<i<<"].size()="<<items[i].size()<<endl;
			return;
		}
	}
	
	// Set number of objects in event
	*tinfo->Nptr = (int)items.size()<tinfo->Nmax ? (int)items.size():tinfo->Nmax;

	// Loop over items in class
	vector<string> tokens;
	unsigned long ptr =  tinfo->Bptr;
	for(unsigned int j=0; j<tinfo->types.size(); j++){
	
		// Set the branch address
		tinfo->branches[j+1]->SetAddress((void*)ptr);
	
		// Loop over objects
		for(unsigned int i=0; i<(unsigned int)*tinfo->Nptr; i++){
			Tokenize(items[i][j].first, tokens, ':');
			if(tokens.size()<3)continue;

			stringstream ss(tokens[2]);

			switch(tinfo->types[j]){
				case type_int:
					ss>>(*(int*)ptr);
					ptr += sizeof(int);
					break;
				case type_uint:
					ss>>(*(unsigned int*)ptr);
					ptr += sizeof(unsigned int);
					break;
				case type_long:
					ss>>(*(long*)ptr);
					ptr += sizeof(long);
					break;
				case type_ulong:
					ss>>(*(unsigned long*)ptr);
					ptr += sizeof(unsigned long);
					break;
				case type_float:
					ss>>(*(float*)ptr);
					ptr += sizeof(float);
					break;
				case type_double:
					ss>>(*(double*)ptr);
					ptr += sizeof(double);
					break;
				case type_string:
					_DBG_<<"String types not implemented yet!"<<endl;
					ptr += 256;
					break;
				default:
					_DBG_<<"Unknown type: "<<tinfo->types[j]<<endl;
					ptr += sizeof(float);
					break;
			}
		}
	}

	// Copy in number of objects
	tinfo->branches[0]->SetAddress((void*)tinfo->Nptr);
#endif
}

