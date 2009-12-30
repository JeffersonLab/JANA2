// $Id$
//
//    File: jc_cmsg.cc
// Created: Sun Dec 27 23:31:21 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include <iostream>
#include <cmath>
using namespace std;

#include <JANA/jerror.h>
#include "jc_cmsg.h"


//---------------------------------
// jc_cmsg    (Constructor)
//---------------------------------
jc_cmsg::jc_cmsg(string myUDL, string myName, string myDescr)
{
	// Initialize mutex
	pthread_mutex_init(&mutex, NULL);

	// Connect to cMsg system
	cMsgSys = new cMsg(myUDL,myName,myDescr);		// the cMsg system object, where
	try {														//  all args are of type string
		cMsgSys->connect();
		is_connected = true;
	} catch (cMsgException e) {
		cout<<endl<<endl<<"_______________  Unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl;
		is_connected = false;
		return;
	}
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);

	// Subscribe to requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Get starting time of high res timer
	start_time = GetTime();

}

//---------------------------------
// ~jc_cmsg    (Destructor)
//---------------------------------
jc_cmsg::~jc_cmsg()
{
	// Unsubscribe
	for(unsigned int i=0; i<subscription_handles.size(); i++){
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();

}

//---------------------------------
// SendCommand
//---------------------------------
void jc_cmsg::SendCommand(string cmd, string subject)
{
	cMsgMessage msg;
	msg.setSubject(subject);
	msg.setType(myname);
	msg.setText(cmd);
	cMsgSys->send(&msg);
	
	cout<<"Sent command: "<<cmd<<" to "<<subject<<endl;
}

//---------------------------------
// PingServers
//---------------------------------
void jc_cmsg::PingServers(void)
{
	SendCommand("who's there?");	
	last_ping_time = GetTime();
}

//---------------------------------
// callback
//---------------------------------
void jc_cmsg::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;

	//cout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();
	if(sender == myname){delete msg; return;} // no need to process messages we sent!

	// Always record times of messages received
	double now = GetTime();
	pthread_mutex_lock(&mutex);
	last_msg_received_time[sender] = now;
	pthread_mutex_unlock(&mutex);

	// The actual command is always sent in the text of the message
	string cmd = msg->getText();
	
	// Dispatch command
	
	//===========================================================
	if(cmd=="who's there?"){
		delete msg;
		return; // We don't actually respond to these, only servers
	}

	//===========================================================
	if(cmd=="I am here"){
		
		// No need to do anything here since sender and time are recorded
		// for all messages automatically.
		
		delete msg;
		return;
	}
	
	//===========================================================

	
	delete msg;
}

//---------------------------------
// ListRemoteProcesses
//---------------------------------
void jc_cmsg::ListRemoteProcesses(void)
{
	// First, ping all remote servers
	PingServers();
	cout<<"Waiting "<<timeout<<" seconds for responses ..."<<endl;
	
	// Sleep for timeout seconds while waiting for responses
	usleep((useconds_t)(timeout*1.0E6));

	// Lock mutex while accessing last_msg_received_time map
	pthread_mutex_lock(&mutex);
	
	// Print results
	cout<<endl;
	cout<<"Processes responding within "<<timeout<<" seconds:"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	map<string, double>::iterator iter=last_msg_received_time.begin();
	for(; iter!=last_msg_received_time.end(); iter++){
		cout<<" "<<iter->first<<"  ("<<(last_ping_time-iter->second)<<" sec.)"<<endl;
	}

	// unlock mutex
	pthread_mutex_unlock(&mutex);
}



