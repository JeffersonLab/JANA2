// $Id$
//
//    File: JEventProcessor_janaroot.cc
// Created: Fri Jul 11 03:39:49 EDT 2008
// Creator: davidl (on Darwin Amelia.local 8.11.1 i386)
//

#include <iostream>
#include <fstream>
using namespace std;

#include "JEventProcessor_janaroot.h"

typedef JEventProcessor_janaroot::TreeInfo TreeInfo;

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->ProvideService(std::make_shared<JLockService>());
	app->Add(new JEventProcessor_janaroot());
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
// JEventProcessor_janaroot (constructor)
//-----------------------------------------
JEventProcessor_janaroot::JEventProcessor_janaroot()
{
	SetTypeName("JEventProcessor_janaroot");
	// Set maximum number of objects a factory can store in an event.
	// Note that this determines the size of the block in memory needed
	// for the the event so be conservative! (This may be overwritten
	// in init below via config. parameter.
	Nmax = 200;

	// Initialize event counter
	Nevents = 0;
	
	// Initialize warnings counter (used in FillTree)
	Nwarnings = 0;
	MaxWarnings=50;
	
	// Set TFile pointert to NULL. It will be created in evnt
	file = NULL;
	
}

//------------------
// Init
//------------------
void JEventProcessor_janaroot::Init(void)
{
	JANAROOT_VERBOSE=0;
	auto app = GetApplication();
	lock_svc = app->GetService<JLockService>();
	app->SetDefaultParameter("JANAROOT_VERBOSE", JANAROOT_VERBOSE);

	app->SetDefaultParameter("JANAROOT_MAX_OBJECTS", Nmax);
}

//------------------
// BeginRun
//------------------
void JEventProcessor_janaroot::BeginRun(const std::shared_ptr<const JEvent>& event)
{
	auto app = GetApplication();
	// Use the ROOT mutex to guarantee that other JEventLoops don't start
	// processing event before we have filled in the nametags_to_write_out member
	lock_svc->RootWriteLock();

	// Allow user to specify factories to write out via configuration parameter
	string factories_to_write_out="";
	app->SetDefaultParameter("WRITEOUT", factories_to_write_out);

	if(factories_to_write_out!=""){
		// Split list at commas
		size_t pos=0;
		string &str = factories_to_write_out;
		while( (pos = str.find(",")) != string::npos ){
			if(pos > 0){
				nametags_to_write_out.push_back(str.substr(0,pos));
			}
			str = str.substr(pos+1);
		}
		if(str.length() > 0){
			nametags_to_write_out.push_back(str);
		}
		
		// Clean up nametags a bit in order to be moe forgiving of exact
		// format user gives us.
		for(unsigned int j=0; j<nametags_to_write_out.size(); j++){
		
			// Strip white space from front and back of nametag
			stringstream ss;
			ss<<nametags_to_write_out[j];
			ss>>nametags_to_write_out[j];
		
			// If the user decides to append a colon ":" to the object name,
			// but with no tag, then we should remove it here so the string
			// is matched properly in evnt().
			if(nametags_to_write_out[j].size()<1)continue;
			size_t pos_last = nametags_to_write_out[j].size()-1;
			if(nametags_to_write_out[j][pos_last] == ':'){
				nametags_to_write_out[j].erase(pos_last);
			}
		}
		
		// At this point we have a list of nametags for factories whose
		// objects we wish to write to the ROOT file. However, this method
		// is only called for one JEventLoop and we can't rely on all others
		// to be up and running yet. So, we have to defer setting the WRITE_TO_OUTPUT
		// flag until evnt time. What we can do here is look to see which
		// factories both exist in the one JEventLoop that is up an running
		// AND were listed in the config parameter.
		
		jout<<"Factories whose objects will be written to ROOT file:"<<endl;
		
		// Loop over all factories
		for (auto factory : event->GetFactorySet()->GetAllFactories()) {
			auto nametag = factory->GetObjectName();
			auto tag = factory->GetTag();

			if (!tag.empty()) nametag += ":" + tag;

			// Look to see if this was one set to write out
			for(unsigned int j=0; j<nametags_to_write_out.size(); j++){
				if(nametags_to_write_out[j] == nametag){
					jout<<"  "<<nametag<<endl;
				}
			}
		}
	}
	
	lock_svc->RootUnLock();
}

//------------------
// Process
//------------------
void JEventProcessor_janaroot::Process(const std::shared_ptr<const JEvent>& event)
{
	// We assume that all factories we want to record have already been
	// activated and so contain the objects (note that the toStrings()
	// method won't create them if they don't exist). Thus, we lock
	// the rootmutex to prevent other threads from accessing the TreeInfo
	// buffers and writing to the ROOT file while we do. Note that
	// this does not prevent other threads from accessing the ROOT
	// globals and mucking things up for us there.
	lock_svc->RootWriteLock();

	// Create ROOT file if not already done
	if( !file ){
		// Remember current working directory. We do this so we can restore it
		// and keep ROOT objects from other parts of the program from
		// showing up in out file.
		TDirectory *cwd = gDirectory;
	
		// Create ROOT file
		file = new TFile("janaroot.root","RECREATE");
	
		// Restore original ROOT directory
		cwd->cd();
	}

	// Get list of all foctories for this JEventLoop to be written out
	vector<JFactory*> allfactories = event->GetFactorySet()->GetAllFactories();
	vector<JFactory*> facs;
	for(unsigned int i=0; i<allfactories.size(); i++){
		// If nametags_to_write_out is empty, we assume the current state
		// of WRITE_TO_OUTPUT flags is valid. Otherwise, we set the flags
		// according to nametags_to_write_out. We do this here rather than
		// in brun since brun is called only once and if more than one thread
		// exists, the other threads will not have their WRITE_TO_OUTPUT
		// flags set properly.
		if(nametags_to_write_out.size()>0){
			allfactories[i]->ClearFactoryFlag(JFactory::WRITE_TO_OUTPUT);
			// Form nametag for factory to compare to ones listed in config. param.
			string nametag = allfactories[i]->GetObjectName();
			string tag = allfactories[i]->GetTag();
			if(!tag.empty()) nametag += ":" + tag;
			for(unsigned int j=0; j<nametags_to_write_out.size(); j++){
				if(nametags_to_write_out[j] == nametag){
					allfactories[i]->SetFactoryFlag(JFactory::WRITE_TO_OUTPUT);
				}
			}
		}

		if(allfactories[i]->TestFactoryFlag(JFactory::WRITE_TO_OUTPUT)) facs.push_back(allfactories[i]);
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
	for(iter=trees.begin(); iter!=trees.end(); iter++)iter->second->tree->Fill();
	
	Nevents++;
	
	// Release the ROOT mutex lock
	lock_svc->RootUnLock();	
}

//------------------
// EndRun
//------------------
void JEventProcessor_janaroot::EndRun(void)
{

}

//------------------
// Finish
//------------------
void JEventProcessor_janaroot::Finish(void)
{
	lock_svc->RootWriteLock();
	if(!file){
		lock_svc->RootUnLock();
	}

	// Remember the current ROOT directory and switch the file
	TDirectory *cwd = gDirectory;
	file->cd();

	// Create a "friend" tree that has no branches of its own, but contains
	// all other trees as friends. This will allow one to plot fields of objects
	// of different types against each other.
	TTree *event = new TTree("event","All objects");
	map<std::string, TreeInfo*>::iterator iter;
	for(iter=trees.begin(); iter!=trees.end(); iter++){
		event->AddFriend(iter->second->tree->GetName());
	}

	// Restore the original ROOT working directory
	cwd->cd();

	// Write the contents of the ROOT file to disk and close it.
	file->Write();
	delete file;
	file=NULL;

	lock_svc->RootUnLock();
}

//------------------
// GetTreeInfo
//------------------
TreeInfo* JEventProcessor_janaroot::GetTreeInfo(JFactory *fac)
{
	// n.b. This gets called while inside the ROOT mutex so it is not locked again here

	// Look for entry in our list of existing branches for this one
	string key = string(fac->GetObjectName())+":"+fac->GetTag();
	map<string, TreeInfo*>::iterator iter = trees.find(key);
	if(iter!=trees.end())return iter->second;
	
	// No branch currently exists. Try creating one.
	// Get factory objects summaries
	auto objs = fac->GetAs<JObject>();
	vector<vector<JObjectMember>> items;
	for (auto obj:objs) {
		JObjectSummary obj_sum;
		obj->Summarize(obj_sum);
		std::vector<JObjectMember> obj_fields = obj_sum.get_fields();
		items.push_back(obj_fields);
	}

	// If no objects currently exists for this factory (either because
	// it was activated, but produced zero objects or was never even
	// activated) then we have no information by which to create a branch.
	// In this case, just return NULL now.
	if(items.size()==0)return NULL;
	
	// Create TreeInfo structure
	TreeInfo *tinfo = new TreeInfo;
	
	// Remember the current ROOT directory and switch to the file
	TDirectory *cwd = gDirectory;
	file->cd();

	// Create tree to hold the objects from this factory.
	string tname = fac->GetObjectName();
	if(fac->GetTag().size()>0)tname += string("_") + fac->GetTag();
	tinfo->tree = new TTree(tname.c_str(),"Autogenerated from JANA objects");
	
	// Restore the original ROOT working directory
	cwd->cd();
	
	// Add "N" branch to hold num objects for the event
	tinfo->branches.push_back(tinfo->tree->Branch("N" , (void*)NULL, "N/I"));
		
	// Loop over data members and extract their name and type to form branches
	tinfo->obj_size=0;
	for(unsigned int i=0; i<items[0].size(); i++){
		// If units are appended using a form like "px(cm)", then chop
		// off the units part and just use the "px".
		auto field_name = items[0][i].name;
		unsigned int cutAt = field_name.find('(');
		string iname = (cutAt==(unsigned int)field_name.npos) ? field_name:field_name.substr(0,cutAt);
		
		// Sometimes the name can have special characters (spaces or "."s)
		// that aren't good for leaf names in branches
		for(size_t k=0; k<iname.size(); k++){
			if(iname[k]==' ')iname[k] = '_';
			if(iname[k]=='.')iname[k] = '_';
		}
		
		unsigned long item_size=0;
		data_type_t type = type_unknown;
		auto field_type = items[0][i].type;
		string branch_def;
		if(field_type=="int"){
			branch_def = iname + "[N]/I";
			item_size = sizeof(int);
			type = type_int;
		}else if(field_type=="uint"){
			branch_def = iname + "[N]/i";
			item_size = sizeof(unsigned int);
			type = type_uint;
		}else if(field_type=="long"){
			branch_def = iname + "[N]/L";
			item_size = sizeof(long);
			type = type_long;
		}else if(field_type=="ulong"){
			branch_def = iname + "[N]/l";
			item_size = sizeof(unsigned long);
			type = type_ulong;
		}else if(field_type=="short"){
			branch_def = iname + "[N]/S";
			item_size = sizeof(short);
			type = type_short;
		}else if(field_type=="ushort"){
			branch_def = iname + "[N]/s";
			item_size = sizeof(unsigned short);
			type = type_ushort;
		}else if(field_type=="float"){
			branch_def = iname + "[N]/F";
			item_size = sizeof(float);
			type = type_float;
		}else if(field_type=="double"){
			branch_def = iname + "[N]/D";
			item_size = sizeof(double);
			type = type_double;
		}else if(field_type=="string"){
			// Strings must be handled differently
			tinfo->StringMap[i]; // Create an element in the map with key "i"
			item_size = 0;
			type = type_string;
		}else{
			branch_def = iname + "[N]/F";
			item_size = sizeof(float);
			type = type_unknown;
		}
		
		// Record the address offest and increase object size
		tinfo->types.push_back(type);
		tinfo->item_sizes.push_back(item_size);
		tinfo->obj_size += item_size;
		
		// Create new branch
		TBranch *branch = NULL;
		if(type == type_string){
			branch = tinfo->tree->Branch(iname.c_str(), &tinfo->StringMap[i]);
		}else{
			branch = tinfo->tree->Branch(iname.c_str(), (void*)NULL, branch_def.c_str());
		}
		tinfo->branches.push_back(branch);
	}
	
	// Inform user if nothing useful in the object is found
	if(tinfo->item_sizes.size()<1){
		_DBG_<<"No usable data members in \""<<tname<<"\"!"<<endl;
	}
	
	// Fill in TreeInfo object
	tinfo->Nmax = Nmax;
	tinfo->buff_size = sizeof(int) + tinfo->Nmax*tinfo->obj_size;
	tinfo->buff = new char[tinfo->buff_size];
	tinfo->Nptr = (int*)tinfo->buff;// "N" is first element in buffer
	tinfo->Bptr = (unsigned long)&tinfo->Nptr[1]; // pointer to buffer starting AFTER "N"
	
	// Add to list
	trees[key] = tinfo;
	
	// At this point we may have already seen any number of events and other classes
	// may have already been added. We want to add Nevents of "NULL" events to the
	// current tree so they will be aligned and we can make them "friends".
	*tinfo->Nptr = 0;
	for(unsigned long i=0; i<Nevents; i++)tinfo->tree->Fill();
	
	if(JANAROOT_VERBOSE>0)tinfo->Print();
	
	return tinfo;
}

//------------------
// FillTree
//------------------
void JEventProcessor_janaroot::FillTree(JFactory *fac, TreeInfo *tinfo)
{
	// n.b. This gets called while inside the ROOT mutex so it is not locked again here
	
	// Get factory objects summaries
	auto objs = fac->GetAs<JObject>();
	vector<vector<JObjectMember>> items;
	for (auto obj:objs) {
		JObjectSummary obj_sum;
		obj->Summarize(obj_sum);
		std::vector<JObjectMember> obj_fields = obj_sum.get_fields();
		items.push_back(obj_fields);
	}
	
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
		
		if(tinfo->branches[j+1] == NULL)continue;
	
		// Set the branch address
		if(tinfo->types[j] == type_string){
			// String address should already be set. We just need to
			// clear the vector so it can be refilled below.
			tinfo->StringMap[j].clear();
		}else{
			tinfo->branches[j+1]->SetAddress((void*)ptr);
		}

		// Loop over objects
		for(unsigned int i=0; i<(unsigned int)*tinfo->Nptr; i++){

			stringstream ss(items[i][j].value);
			string str;

			switch(tinfo->types[j]){
				case type_short:
					ss>>(*(short*)ptr);
					break;
				case type_ushort:
					ss>>(*(unsigned short*)ptr);
					break;
				case type_int:
					ss>>(*(int*)ptr);
					break;
				case type_uint:
					ss>>(*(unsigned int*)ptr);
					break;
				case type_long:
					ss>>(*(long*)ptr);
					break;
				case type_ulong:
					ss>>(*(unsigned long*)ptr);
					break;
				case type_float:
					ss>>(*(float*)ptr);
					break;
				case type_double:
					ss>>(*(double*)ptr);
					break;
				case type_string:
					str = items[i][j].value;
					//ss>>str;
					tinfo->StringMap[j].push_back(str);
					break;
				default:
					if(Nwarnings<MaxWarnings && JANAROOT_VERBOSE>0){
						_DBG_<<"Unknown type: "<<tinfo->types[j];
						Nwarnings++;
						if(Nwarnings==MaxWarnings)cerr<<" --last warning! --";
						cerr<<endl;
					}
					break;
			}
			ptr += tinfo->item_sizes[j];
		}
	}

	// Copy in number of objects
	tinfo->branches[0]->SetAddress((void*)tinfo->Nptr);
}

