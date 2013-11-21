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
	if(cmd=="sources list"){
		// Extract data from message
		vector<string> *classNames = NULL;
		vector<string> *sourceNames  = NULL;
		if(msg->payloadContainsName("classNames")) classNames = msg->getStringVector("classNames");
		if(msg->payloadContainsName("sourceNames")) sourceNames = msg->getStringVector("sourceNames");

		// If no active sources then the pointers will be NULL. This is not
		// necessarily an error.
		vector<pair<string, string> > mysources;
		if(classNames != NULL && sourceNames!=NULL){

			if(classNames->size() != sourceNames->size()){
				cerr << "ERROR: classNames and sourceNames vector sizes don't match!!" << endl;
				exit(-2);
			}

			for(unsigned int i=0; i<classNames->size(); i++){
				mysources.push_back(make_pair((*classNames)[i], (*sourceNames)[i]));
			}
		}
	
		pthread_mutex_lock(&mutex);
		sources[sender] = mysources;
		pthread_mutex_unlock(&mutex);

		delete msg;
		return;
	}

	//===========================================================
	if(cmd=="host info"){
		// Extract data from message
		vector<string> *keys = NULL;
		vector<string> *vals  = NULL;
		if(msg->payloadContainsName("keys")) keys = msg->getStringVector("keys");
		if(msg->payloadContainsName("vals")) vals = msg->getStringVector("vals");

		// If no active sources then the pointers will be NULL. This is not
		// necessarily an error.
		vector<pair<string, string> > myhostInfos;
		if(keys != NULL && vals!=NULL){

			if(keys->size() != vals->size()){
				cerr << "ERROR: keys and vals vector sizes don't match!!" << endl;
				exit(-2);
			}

			for(unsigned int i=0; i<keys->size(); i++){
				myhostInfos.push_back(make_pair((*keys)[i], (*vals)[i]));
			}
		}
	
		pthread_mutex_lock(&mutex);
		hostInfos[sender] = myhostInfos;
		pthread_mutex_unlock(&mutex);

		delete msg;
		return;
	}

	//===========================================================
	if(cmd=="command line"){
		// Extract data from message
		if(msg->payloadContainsName("command")){
			string command = msg->getString("command");
	
			pthread_mutex_lock(&mutex);
			commandLines.push_back(pair<string,string>(sender, command));
			pthread_mutex_unlock(&mutex);
		}

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

//---------------------------------
// ListSources
//---------------------------------
void jc_cmsg::ListSources(string subject)
{
	SendCommand("list sources", subject);
	
	// Wait for timeout seconds for all clients to respond. Sleep for
	// 100 ms increments at a time while waiting.
	double time_slept = 0.0;
	double time_to_sleep_per_iteration = 0.1;
	do{
		usleep((int)(time_to_sleep_per_iteration*1.0E6));
		time_slept += time_to_sleep_per_iteration;
		if(subject!="janactl" && sources.size()>0)break;
	}while(time_slept<timeout);

	// Lock mutex
	pthread_mutex_lock(&mutex);

	// Find longest responder name
	map<string, vector<pair<string, string> > >::iterator iter;
	size_t max_len = 0;
	for(iter = sources.begin(); iter!=sources.end(); iter++){
		size_t len = iter->first.length();
		if(len > max_len) max_len = len;
	}
	max_len++;

	// Print results
	cout<<endl;
	cout<<"Active Event Sources:"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	for(iter = sources.begin(); iter!=sources.end(); iter++){
		const string &responder = iter->first;
		const vector<pair<string, string> > &mysources = iter->second;

		for(unsigned int i=0; i<mysources.size(); i++){
			const string &className = mysources[i].first;
			const string &sourceName = mysources[i].second;
			if(i==0){
				cout << string(max_len - responder.length(), ' ') << responder;
			}else{
				cout << string(max_len, ' ');
			}
			cout << "   >   "<< className << " : " << sourceName << endl;
		}
	}
	cout<<endl;

	// unlock mutex
	pthread_mutex_unlock(&mutex);
}

//---------------------------------
// ListSourceTypes
//---------------------------------
void jc_cmsg::ListSourceTypes(string subject)
{

}

//---------------------------------
// ListFactories
//---------------------------------
void jc_cmsg::ListFactories(string subject)
{

}

//---------------------------------
// ListPlugins
//---------------------------------
void jc_cmsg::ListPlugins(string subject)
{

}

//---------------------------------
// GetCommandLine
//---------------------------------
void jc_cmsg::GetCommandLine(string subject)
{
	/// Get command line used to launch remote process(es)

	SendCommand("command line", subject);

	// Wait for timeout seconds for all clients to respond. Sleep for
	// 100 ms increments at a time while waiting.
	double time_slept = 0.0;
	double time_to_sleep_per_iteration = 0.1;
	do{
		usleep((int)(time_to_sleep_per_iteration*1.0E6));
		time_slept += time_to_sleep_per_iteration;
		if(subject!="janactl" && commandLines.size()>0)break;
	}while(time_slept<timeout);

	// Lock mutex
	pthread_mutex_lock(&mutex);

	// Print results
	cout<<endl;
	cout<<"Command Lines:"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	vector<pair<string, string> >::iterator iter;
	for(iter = commandLines.begin(); iter!=commandLines.end(); iter++){
		const string &responder = iter->first;
		const string &commandLine = iter->second;

		cout << " " << responder << " : " << commandLine << endl;
	}
	cout<<endl;

	// unlock mutex
	pthread_mutex_unlock(&mutex);
}

//---------------------------------
// GetHostInfo
//---------------------------------
void jc_cmsg::GetHostInfo(string subject)
{
	/// The remote process will repond with two vectors representing
	/// key/value pairs which are then just printed. This allows the
	/// remote process to decide what info it sends.

	SendCommand("host info", subject);
	
	// Wait for timeout seconds for all clients to respond. Sleep for
	// 100 ms increments at a time while waiting.
	double time_slept = 0.0;
	double time_to_sleep_per_iteration = 0.1;
	do{
		usleep((int)(time_to_sleep_per_iteration*1.0E6));
		time_slept += time_to_sleep_per_iteration;
		if(subject!="janactl" && hostInfos.size()>0)break;
	}while(time_slept<timeout);

	// Lock mutex
	pthread_mutex_lock(&mutex);

	// Find longest key
	map<string, vector<pair<string, string> > >::iterator iter;
	size_t max_len = 0;
	size_t max_key_len = 0;
	for(iter = hostInfos.begin(); iter!=hostInfos.end(); iter++){
		size_t len = iter->first.length();
		if(len > max_len) max_len = len;

		const vector<pair<string, string> > &myinfos = iter->second;
		for(unsigned int i=0; i<myinfos.size(); i++){
			size_t len = myinfos[i].first.length();
			if(len > max_key_len) max_key_len = len;
		}
	}
	max_len++;
	max_key_len++;

	// Print results
	cout<<endl;
	cout<<"Host info:"<<endl;
	cout<<"---------------------------------------------------"<<endl;
	for(iter = hostInfos.begin(); iter!=hostInfos.end(); iter++){
		const string &responder = iter->first;
		const vector<pair<string, string> > &myinfos = iter->second;

		cout << "   " << responder << ":" << endl;
		cout<<"   -----------------------------"<<endl;
		for(unsigned int i=0; i<myinfos.size(); i++){
			const string &key = myinfos[i].first;
			const string &value = myinfos[i].second;

			cout << string(max_key_len - key.length(), ' ');
			cout << "     "<< key << " : " << value << endl;
		}
	}
	cout<<endl;

	// unlock mutex
	pthread_mutex_unlock(&mutex);
}

////---------------------------------
//// AttachPlugin
////---------------------------------
//void jc_cmsg::AttachPlugin(string subject, string plugin)
//{
//	string command = "attach plugin " + plugin;
//	SendCommand(command, subject);
//
//	// Sometimes, the user will include the ".so" suffix in the
//	// plugin name. If they don't, then we add it here.
//	if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";
//
//	// Loop over paths
//	bool found_plugin=false;
//	for(unsigned int i=0; i< pluginPaths.size(); i++){
//		string fullpath = pluginPaths[i] + "/" + plugin;
//		ifstream f(fullpath.c_str());
//		if(f.is_open()){
//			f.close();
//			if(RegisterSharedObject(fullpath.c_str())==NOERROR)found_plugin=true;
//			break;
//		}
//		if(printPaths) jout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
//		
//		if(fullpath[0] != '/')continue;
//		fullpath = pluginPaths[i] + "/" + plugins[j] + "/" + plugin;
//		f.open(fullpath.c_str());
//		if(f.is_open()){
//			f.close();
//			if(RegisterSharedObject(fullpath.c_str())==NOERROR)found_plugin=true;
//			break;
//		}
//		if(printPaths) jout<<"Looking for \""<<fullpath<<"\" ...."<<"no"<<endl;
//	}
//	
//	// If we didn't find the plugin, then complain and quit
//	if(!found_plugin){
//		Lock();
//		jerr<<endl<<"***ERROR : Couldn't find plugin \""<<plugins[j]<<"\"!***"<<endl;
//		jerr<<"***        To see paths checked, set PRINT_PLUGIN_PATHS config. parameter"<<endl;
//		Unlock();
//		exit(-1);
//	}
//}


#endif //HAVE_CMSG

