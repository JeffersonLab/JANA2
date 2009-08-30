// $Id$
//
//    File: rs_cmsg.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include <iostream>
#include <cmath>
using namespace std;

#include <TDirectoryFile.h>
#include <TMessage.h>
#include <TH1.h>

#include "RootSpy.h"
#include "rs_cmsg.h"
#include "rs_info.h"

// See http://www.jlab.org/Hall-D/software/wiki/index.php/Serializing_and_deserializing_root_objects
class MyTMessage : public TMessage {
public:
   MyTMessage(void *buf, Int_t len) : TMessage(buf, len) { }
};


//---------------------------------
// rs_cmsg    (Constructor)
//---------------------------------
rs_cmsg::rs_cmsg(void)
{
	// Connect to cMsg system
	string myUDL = "cMsg://localhost/cMsg/rootspy";
	string myName = "RootSpy";
	string myDescr = "Access ROOT objects in a running program";
	cMsgSys = new cMsg(myUDL,myName,myDescr);      // the cMsg system object, where
	try {                                    //  all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl<<endl<<endl;
		return;
	}
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);

	cout<<"---------------------------------------------------"<<endl;
	cout<<"rootspy name: \""<<myname<<"\""<<endl;
	cout<<"---------------------------------------------------"<<endl;

	// Subscribe to generic rootspy info requests
	subscription_handles.push_back(cMsgSys->subscribe("rootspy", "*", this, NULL));

	// Subscribe to rootspy requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Broadcast request for available servers
	PingServers();
}

//---------------------------------
// ~rs_cmsg    (Destructor)
//---------------------------------
rs_cmsg::~rs_cmsg()
{
	// Unsubscribe
	for(unsigned int i=0; i<subscription_handles.size(); i++){
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();

}

//---------------------------------
// PingServers
//---------------------------------
void rs_cmsg::PingServers(void)
{
	cMsgMessage whosThere;
	whosThere.setSubject("rootspy");
	whosThere.setType(myname);
	whosThere.setText("who's there?");
	
	cMsgSys->send(&whosThere);
}

//---------------------------------
// RequestHists
//---------------------------------
void rs_cmsg::RequestHists(string servername)
{
	cMsgMessage listHists;
	listHists.setSubject(servername);
	listHists.setType(myname);
	listHists.setText("list hists");
	
	cMsgSys->send(&listHists);
}

//---------------------------------
// RequestHistogram
//---------------------------------
void rs_cmsg::RequestHistogram(string servername, string hnamepath)
{
	cMsgMessage requestHist;
	requestHist.setSubject(servername);
	requestHist.setType(myname);
	requestHist.setText("get hist");
	requestHist.add("hnamepath", hnamepath);
	
	cMsgSys->send(&requestHist);
}

//---------------------------------
// callback
//---------------------------------
void rs_cmsg::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;

	//cout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();
	if(sender == myname)return; // no need to process messages we sent!
	
	// The actual command is always sent in the text of the message
	string cmd = msg->getText();
	
	// Update server list with time of this message
	RS_INFO->Lock();
	RS_INFO->servers[sender] = time(NULL);
	RS_INFO->Unlock();
	
	// Dispatch command
	
	//===========================================================
	if(cmd=="who's there?"){
		return; // We don't actually respond to these, only servers
	}

	//===========================================================
	if(cmd=="I am here"){
		if(RS_INFO->selected_server=="N/A")RS_INFO->selected_server=sender;
		return;
	}
	
	//===========================================================
	if(cmd=="hists list"){
		vector<string> *hist_names = msg->getStringVector("hist_names");
		vector<string> *hist_types = msg->getStringVector("hist_types");
		vector<string> *hist_paths = msg->getStringVector("hist_paths");
		vector<string> *hist_titles = msg->getStringVector("hist_titles");
		bool good_response = hist_names!=NULL && hist_types!=NULL && hist_paths!=NULL && hist_titles!=NULL;
		if(good_response)good_response = hist_names->size()==hist_types->size();
		if(good_response)good_response = hist_names->size()==hist_paths->size();
		if(good_response)good_response = hist_names->size()==hist_titles->size();
		if(!good_response){
			_DBG_<<"Poorly formed repsonse for \"hists list\". Ignoring."<<endl;
			return;
		}
		vector<rs_info::hinfo_t> hinfos;
		for(unsigned int i=0; i<hist_names->size(); i++){
			rs_info::hinfo_t hi;
			hi.name = (*hist_names)[i];
			hi.type = (*hist_types)[i];
			hi.path = (*hist_paths)[i];
			hi.title = (*hist_titles)[i];
			hinfos.push_back(hi);
		}
		RS_INFO->hists[sender] = hinfos;

		return;
	}

	//===========================================================
	if(cmd=="histogram"){
		MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
		TNamed *namedObj = (TNamed*)myTM->ReadObject(myTM->GetClass());
		if(!namedObj)return;
		string className = namedObj->ClassName();
		//cout << "Received object ClassName is:  " << className << endl;
		
		TH1 *h = dynamic_cast<TH1*>(namedObj);
		if(h){
			RS_INFO->Lock();
			if(RS_INFO->latest_hist)delete RS_INFO->latest_hist;
			RS_INFO->latest_hist = h;
			RS_INFO->latest_hist_received_time = time(NULL);
			RS_INFO->Unlock();
		}else{
			delete namedObj;
		}
	}
}

