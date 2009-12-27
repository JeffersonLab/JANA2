// $Id$
// janactl_plugin.cc
// Created: Mon Dec 21 23:45:22 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include <iostream>
#include <cmath>
using namespace std;

#include <JANA/JApplication.h>
using namespace jana;

#include <janactl_plugin.h>


// Entrance point for plugin
extern "C"{
void InitPlugin(JApplication *japp){
	InitJANAPlugin(japp);

#if HAVE_CMSG

	new janactl_plugin(japp);

#else

	jerr<<endl;
	jerr<<"You are attempting to use the janactl plugin compiled without"<<endl;
	jerr<<"cMsg support. cMsg is required to use janactl. cMsg can be obtained"<<endl;
	jerr<<"from the JLab ftp site: ftp.jlab.org in the pub/coda/cMsg directory."<<endl;
	jerr<<"Please install cMsg, re-run the configure script and rebuild JANA"<<endl;
	jerr<<"to enable this feature."<<endl;
	jerr<<endl;

#endif // HAVE_CMSG

}
}

#if HAVE_CMSG


#ifndef _DBG_
#define _DBG_ jerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ jerr<<__FILE__<<":"<<__LINE__<<endl;
#endif

//---------------------------------
// janactl_plugin    (Constructor)
//---------------------------------
janactl_plugin::janactl_plugin(JApplication *japp)
{
	this->japp = japp;
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);

	// Connect to cMsg system
	string myUDL = "cMsg://localhost/cMsg/janactl";
	string myName = myname;
	string myDescr = "Allow external monitoring/control of a JANA program";
	
	JParameterManager *parms = japp->GetJParameterManager();
	parms->SetDefaultParameter("JANACTL:UDL", myUDL);
	parms->SetDefaultParameter("JANACTL:Name", myName);
	parms->SetDefaultParameter("JANACTL:Description", myDescr);
	
	cMsgSys = new cMsg(myUDL,myName,myDescr);      // the cMsg system object, where
	try {                                    //  all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		jerr<<endl<<"_______________  janactl unable to connect to cMsg system! __________________"<<endl;
		jerr<<"         UDL : "<<myUDL<<endl;
		jerr<<"        name : "<<myName<<endl;
		jerr<<" description : "<<myDescr<<endl;
		jerr<<endl;
		jerr << e.toString() << endl; 
		jerr<<endl;
		jerr<<"Make sure the UDL is correct (you may set it via the JANACTL:UDL config. parameter)"<<endl;
		jerr<<"You may also override the default, generated name with JANACTL:Name and the description"<<endl;
		jerr<<"with JANACTL:Description."<<endl;
		return;
	}

	jout<<"---------------------------------------------------"<<endl;
	jout<<"janactl name: \""<<myname<<"\""<<endl;
	jout<<"---------------------------------------------------"<<endl;

	// Subscribe to generic janactl info requests
	subscription_handles.push_back(cMsgSys->subscribe("janactl", "*", this, NULL));

	// Subscribe to janactl requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Broadcast that we're here to anyone already listening
	cMsgMessage ImAlive;
	ImAlive.setSubject("janactl");
	ImAlive.setType(myname);
	cMsgSys->send(&ImAlive);
}

//---------------------------------
// ~janactl_plugin    (Destructor)
//---------------------------------
janactl_plugin::~janactl_plugin()
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
void janactl_plugin::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;

	//jout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

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

	//======================================================================
	if(cmd=="who's there?"){
		response.setText("I am here");
		cMsgSys->send(&response);
		delete msg;
		return;
	}
	
	//======================================================================
	if(cmd=="quit"){
	
		// Tell JApplication to quit
		japp->Quit();
		
		delete msg;
		return;
	}
	
	//======================================================================
	if(cmd=="get threads"){
#if 0
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
#endif
		response.setText("hists list");
		cMsgSys->send(&response);

		delete msg;
		return;
	}
	
	delete msg;
}


#endif // HAVE_CMSG

