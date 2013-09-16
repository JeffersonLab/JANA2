// $Id$
//
//    File: jc_cmsg.cc
// Created: Sun Dec 27 23:31:21 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>
#include <stdio.h>

#include <iostream>
#include <cmath>
using namespace std;

#include <JANA/jerror.h>
#include "jc_cmsg.h"

#if HAVE_CMSG

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

	cout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

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
	if(cmd=="thread info"){
		// Extract data from message
		vector<uint64_t> *threads = msg->getUint64Vector("threads");
		vector<double> *instantaneous_rates = msg->getDoubleVector("instantaneous_rates");
		vector<double> *average_rates = msg->getDoubleVector("average_rates");
		vector<uint64_t> *nevents = msg->getUint64Vector("nevents");
		
		if(threads->size() != instantaneous_rates->size()
			|| threads->size() != average_rates->size()
			|| nevents->size() != nevents->size()){
			
			delete msg;
			return;
		}
		
		vector<thrinfo_t> my_thrinfos;
		for(unsigned int i=0; i<threads->size(); i++){
			thrinfo_t t;
			t.thread = (*threads)[i];
			t.Nevents = (*nevents)[i];
			t.rate_instantaneous = (*instantaneous_rates)[i];
			t.rate_average = (*average_rates)[i];
			my_thrinfos.push_back(t);
		}

		pthread_mutex_lock(&mutex);
		thrinfos[sender] = my_thrinfos;
		pthread_mutex_unlock(&mutex);

		delete msg;
		return;
	}

	//===========================================================
	if(cmd=="configuration parameter list"){
		// Extract data from message
//		msg->payloadPrint();
		vector<string> *names = msg->getStringVector("names");
		vector<string> *vals  = msg->getStringVector("vals");
		
		if(names->size() != vals->size()){
			cerr << "ERROR: names and vals vector sizes don't match!!" << endl;
			exit(-2);
		}
		
		pthread_mutex_lock(&mutex);
		config_params_responder = sender;
		for(unsigned int i=0; i<names->size(); i++){
			config_params[(*names)[i]] = (*vals)[i];
		}
		pthread_mutex_unlock(&mutex);

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

//---------------------------------
// GetThreadInfo
//---------------------------------
void jc_cmsg::GetThreadInfo(string subject)
{
	SendCommand("get threads", subject);
	last_threadinfo_time = GetTime();
	
	// If subject is janactl then assume we're sending this to multiple recipients
	// so we must wait the full "timeout". Otherwise, we want to continue as soon
	// as we get the first response. To make this happen, we sleep for 100 us increments
	// at a time.
	double time_slept = 0.0;
	double time_to_sleep_per_iteration = 0.1;
	do{
		usleep((int)(time_to_sleep_per_iteration*1.0E6));
		time_slept += time_to_sleep_per_iteration;
		if(subject!="janactl" && thrinfos.size()>0)break;
	}while(time_slept<timeout);

	// Lock mutex
	pthread_mutex_lock(&mutex);
	
	// Print results
	cout<<endl;
	cout<<"Threads by process:"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	map<string, vector<thrinfo_t> >::iterator iter=thrinfos.begin();
	for(; iter!=thrinfos.end(); iter++){
		cout<<iter->first<<":"<<endl;
		vector<thrinfo_t> &thrinfo = iter->second;
		for(unsigned int i=0; i<thrinfo.size(); i++){
			thrinfo_t &t = thrinfo[i];
			cout<<"   thread: "<<i<<" 0x"<<hex<<t.thread<<dec<<"  "<<t.Nevents<<" events  "<<t.rate_instantaneous<<"Hz ("<<t.rate_average<<"Hz avg.)"<<endl;
		}
	}

	// unlock mutex
	pthread_mutex_unlock(&mutex);
	
}

//---------------------------------
// ListConfigurationParameters
//---------------------------------
void jc_cmsg::ListConfigurationParameters(string subject)
{
	SendCommand("list configuration parameters", subject);
	//last_threadinfo_time = GetTime();
	
	// If subject is janactl then assume we're sending this to multiple recipients
	// so we must wait the full "timeout". Otherwise, we want to continue as soon
	// as we get the first response. To make this happen, we sleep for 100 ms increments
	// at a time.
	double time_slept = 0.0;
	double time_to_sleep_per_iteration = 0.1;
	do{
		usleep((int)(time_to_sleep_per_iteration*1.0E6));
		time_slept += time_to_sleep_per_iteration;
		if(config_params.size()>0)break;
	}while(time_slept<timeout);

	// Lock mutex
	pthread_mutex_lock(&mutex);
	
	// Get length of longest parameter name for pretty printing
	map<string, string >::iterator iter=config_params.begin();
	size_t max_len = 0;
	for(; iter!=config_params.end(); iter++){
		size_t len = iter->first.length();
		if(len > max_len) max_len = len;
	}
	max_len++;
	
	// Print results
	cout<<endl;
	cout<<"Configuration Parameters (from "<<config_params_responder<<"):"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	iter=config_params.begin();
	for(; iter!=config_params.end(); iter++){
		size_t len = iter->first.length();
		if(max_len > len) cout<<string(max_len-len, ' '); // indent so that "=" characters line up
		cout<<iter->first<<" = ";
		cout<<iter->second<<endl;
	}
	cout<<endl;

	// unlock mutex
	pthread_mutex_unlock(&mutex);
}

#endif //HAVE_CMSG

