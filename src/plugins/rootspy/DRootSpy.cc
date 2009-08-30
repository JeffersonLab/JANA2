// $Id$
//
//    File: DRootSpy.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include <iostream>
#include <cmath>
using namespace std;

#include <TROOT.h>
#include <TDirectoryFile.h>
#include <TClass.h>
#include <TH1.h>
#include <TMessage.h>

#include <JANA/JApplication.h>
#include <JANA/JEventLoop.h>
using namespace jana;


#include "DRootSpy.h"


// Entrance point for plugin
extern "C"{
void InitPlugin(JApplication *japp){
	InitJANAPlugin(japp);
	
	new DRootSpy();
}
}

//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
DRootSpy::DRootSpy(void)
{
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);

	// Connect to cMsg system
	string myUDL = "cMsg://localhost/cMsg/rootspy";
	string myName = myname;
	string myDescr = "Access ROOT objects in JANA program";
	cMsgSys = new cMsg(myUDL,myName,myDescr);      // the cMsg system object, where
	try {                                    //  all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl<<endl<<endl;
		return;
	}

	cout<<"---------------------------------------------------"<<endl;
	cout<<"rootspy name: \""<<myname<<"\""<<endl;
	cout<<"---------------------------------------------------"<<endl;

	// Subscribe to generic rootspy info requests
	subscription_handles.push_back(cMsgSys->subscribe("rootspy", "*", this, NULL));

	// Subscribe to rootspy requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Broadcast that we're here to anyone already listening
	cMsgMessage ImAlive;
	ImAlive.setSubject("rootspy");
	ImAlive.setType(myname);
	cMsgSys->send(&ImAlive);
}

//---------------------------------
// ~DRootSpy    (Destructor)
//---------------------------------
DRootSpy::~DRootSpy()
{
	// Unsubscribe
	for(unsigned int i=0; i<subscription_handles.size(); i++){
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();

}

//---------------------------------
// callback
//---------------------------------
void DRootSpy::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;

	//cout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();
	
	// The actual command is always sent in the text of the message
	string cmd = msg->getText();
	
	// Prepare to send a response (this may or may not be needed below, depending on the command)
	cMsgMessage response;
	response.setSubject(sender);
	response.setType(myname);

	// Dispatch command 
	if(cmd=="who's there?"){
		response.setText("I am here");
		cMsgSys->send(&response);
		return;
	}
	
	//======================================================================
	if(cmd=="list hists"){
		// Add histograms to list, recursively traversing ROOT directories
		vector<hinfo_t> hinfos;
		AddRootObjectsToList(gDirectory, hinfos);
	
		// If any histograms were found, copy their info into the message
		if(hinfos.size()>0){
			// Copy elements of hinfo objects into individual vectors
			vector<string> hist_names;
			vector<string> hist_types;
			vector<string> hist_paths;
			vector<string> hist_titles;
			for(unsigned int i=0; i<hinfos.size(); i++){
				hist_names.push_back(hinfos[i].name);
				hist_types.push_back(hinfos[i].type);
				hist_paths.push_back(hinfos[i].path);
				hist_titles.push_back(hinfos[i].title);
			}
			response.add("hist_names", hist_names);
			response.add("hist_types", hist_types);
			response.add("hist_paths", hist_paths);
			response.add("hist_titles", hist_titles);
		}

		response.setText("hists list");
		cMsgSys->send(&response);

		return;
	}
	
	//======================================================================
	if(cmd=="get hist"){
		// Get name of requested histogram
		string hnamepath = msg->getString("hnamepath");
		
		// split hnamepath into histo name and path
		size_t pos = hnamepath.find_last_of("/");
		if(pos == string::npos)return;
		string hname = hnamepath.substr(pos+1, hnamepath.length()-1);
		string path = hnamepath.substr(0, pos);
		if(path[path.length()-1]==':')path += "/";

		// This seems to work, but is certainly not very thread safe.
		// perhaps there's a better way...?
		TDirectory *savedir = gDirectory;
		gROOT->cd(path.c_str());
		TObject *obj = gROOT->FindObject(hname.c_str());
		savedir->cd();
		if(!obj)return;
		
		// Serialize object and put into response message
		TMessage *tm = new TMessage(kMESS_OBJECT);
		tm->WriteObject(obj);
		response.setByteArrayNoCopy(tm->Buffer(),tm->Length());

		response.setText("histogram");
		cMsgSys->send(&response);

		return;
	}
}

//---------------------------------
// AddRootObjectsToMessage
//---------------------------------
void DRootSpy::AddRootObjectsToList(TDirectory *dir, vector<hinfo_t> &hinfos)
{
	string path = dir->GetPath();

	TList *list = dir->GetList();
	TIter next(list);
	while(TObject *obj = next()){
		
		// For now, we only report objects inheriting from TH1
		TH1 *h = dynamic_cast<TH1*>(obj);
		if(h!=NULL){
			hinfo_t hi;
			hi.name = obj->GetName();
			hi.type = obj->ClassName();
			hi.path = path;
			hi.title = h->GetTitle();
			hinfos.push_back(hi);
		}
		
		// If this happens to be a directory, recall ourself to find objects starting from it
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if(subdir!=NULL && subdir!=dir)AddRootObjectsToList(subdir, hinfos);
	}
}


