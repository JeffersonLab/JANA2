// $Id$
// janactl_plugin.cc
// Created: Mon Dec 21 23:45:22 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#ifdef __APPLE__
#define HAVE_SYSCTL
#include <sys/sysctl.h>
#endif // __APPLE__

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
janactl_plugin::janactl_plugin(JApplication *japp):jctlout(std::cout, "JANACTL>>")
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
	
	cMsgSys = new cMsg(myUDL,myName,myDescr);	// the cMsg system object, where
	try {                                    	// all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		jctlout<<endl<<"_______________  janactl unable to connect to cMsg system! __________________"<<endl;
		jctlout<<"         UDL : "<<myUDL<<endl;
		jctlout<<"        name : "<<myName<<endl;
		jctlout<<" description : "<<myDescr<<endl;
		jctlout<<endl;
		jctlout << e.toString() << endl; 
		jctlout<<endl;
		jctlout<<"Make sure the UDL is correct (you may set it via the JANACTL:UDL config. parameter)"<<endl;
		jctlout<<"You may also override the default, generated name with JANACTL:Name and the description"<<endl;
		jctlout<<"with JANACTL:Description."<<endl;
		return;
	}

	jctlout<<"---------------------------------------------------"<<endl;
	jctlout<<"janactl name: \""<<myname<<"\""<<endl;
	jctlout<<"---------------------------------------------------"<<endl;

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

	jout<<"Received message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

	// The convention here is that the message "type" always contains the
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
	if(cmd=="kill"){
		
		jctlout<<endl<<"Killing application ..."<<endl;
	
		// Kill program immediately
		exit(-1);
		
		delete msg;
		return;
	}
	
	//======================================================================
	if(cmd=="quit"){
		
		jctlout<<endl<<"Quitting application ..."<<endl;
	
		// Tell JApplication to quit
		japp->Quit();
		
		delete msg;
		return;
	}

	//======================================================================
	if(cmd=="pause"){
		
		jctlout<<endl<<"Pausing event processing ..."<<endl;
	
		// Tell JApplication to quit
		japp->Pause();
		
		delete msg;
		return;
	}

	//======================================================================
	if(cmd=="resume"){
		
		jctlout<<endl<<"Resuming  event processing ..."<<endl;
	
		// Tell JApplication to quit
		japp->Resume();
		
		delete msg;
		return;
	}
	
	//======================================================================
	if(cmd=="get threads"){
		map<pthread_t,double> rate_instantaneous;
		map<pthread_t,double> rate_average;
		map<pthread_t,unsigned int> nevents;
		japp->GetInstantaneousThreadRates(rate_instantaneous);
		japp->GetIntegratedThreadRates(rate_average);
		japp->GetThreadNevents(nevents);
		
		// Containers to hold thread info
		vector<uint64_t> threads;
		vector<double> instantaneous_rates;
		vector<double> average_rates;
		vector<uint64_t> nevents_by_thread;
		
		// Loop over nevents by thread
		map<pthread_t,unsigned int>::iterator nevents_iter= nevents.begin();
		for(; nevents_iter!=nevents.end(); nevents_iter++){
			// Only include threads for which we have all info
			map<pthread_t,double>::iterator irate = rate_instantaneous.find(nevents_iter->first);
			map<pthread_t,double>::iterator arate = rate_average.find(nevents_iter->first);
			if(irate==rate_instantaneous.end() || arate==rate_average.end())continue;
			
			threads.push_back((uint64_t)nevents_iter->first);
			nevents_by_thread.push_back(nevents_iter->second);
			instantaneous_rates.push_back(irate->second);
			average_rates.push_back(arate->second);
		}
		
		response.add("threads", &threads);
		response.add("instantaneous_rates", &instantaneous_rates);
		response.add("average_rates", &average_rates);
		response.add("nevents", &nevents_by_thread);

		response.setText("thread info");
		cMsgSys->send(&response);

		delete msg;
		return;
	}

	//======================================================================
	if(cmd.find("killthread ")==0){

		// Extract thread number from command
		stringstream ss(cmd.substr(11));
		pthread_t thr;

		// If value contains an "x" assume it is in hex form and
		// represents the pthread_t value. Otherwise, assume it
		// is an index to the i-th thread.
		if(ss.str().find("x")!=string::npos){
			ss >> hex >> *(unsigned long*)&thr;
		}else{
			int ithr=0;
			ss >> ithr;
			thr = japp->GetThreadID(ithr);
		}

		// Kill the thread
		bool found_thread = japp->KillThread(thr);
		if(found_thread){
			response.setText("OK"); // send command
		}else{
			response.setText("Thread not found!"); // send command
		}
		cMsgSys->send(&response);

		delete msg;
		return;		
	}

	//======================================================================
	if(cmd.find("set nthreads ")==0){

		// Extract number of threads from command
		stringstream ss(cmd.substr(13));

		// Extract the desired thread number
		int Nthreads=0;
		ss >> Nthreads;
		if(Nthreads>0){
			japp->SetNthreads(Nthreads);
			response.setText("OK"); // send command
		}else{
			response.setText("BAD value for nthreads"); // send command
		}
		cMsgSys->send(&response);

		delete msg;
		return;		
	}

	//======================================================================
	if(cmd.find("list configuration parameters")==0){
		
		if(gPARMS != NULL){
			
			map<string,string> parms;
			gPARMS->GetParameters(parms);
			vector<string> names;
			vector<string> vals;
			for(map<string,string>::iterator iter=parms.begin(); iter!=parms.end(); iter++){
				string val = iter->second;
				if(val == "") val="<empty>"; // cMsg bug causes problems if empty string is sent
				names.push_back(iter->first);
				vals.push_back(val);
			}

			response.setText("configuration parameter list");
			response.add("names", names);
			response.add("vals" , vals);

		}else{
			response.setText("gPARMS is NULL!");
		}

		cMsgSys->send(&response);

		delete msg;
		return;		
	}

	//======================================================================
	if(cmd.find("list sources")==0){

		vector<string> classNames;
		vector<string> sourceNames;

		japp->GetActiveEventSourceNames(classNames, sourceNames);

		response.setText("sources list");
		if(classNames.size() > 0){
			response.add("classNames" , classNames);
			response.add("sourceNames", sourceNames);
		}

		cMsgSys->send(&response);

		delete msg;
		return;		
	}

	//======================================================================
	if(cmd.find("host info")==0){

		vector<string> keys;
		vector<string> vals;

		// There does not seem to be a standard way to get detailed system
		// info that works on both Linux and Mac OS X. Thus, use preprocessor
		// directives to decide which routine to call to fill the keys, vals
		// vectors.

#ifdef HAVE_PROC
		HostInfoPROC(keys, vals);
#endif // HAVE_PROC

#ifdef HAVE_SYSCTL
		HostInfoSYSCTL(keys, vals);
#endif // HAVE_SYSCTL

		// Make sure we have the same number of keys and values
		if(keys.size() != vals.size()){
			jctlout << "keys.size() != vals.size() when returning host info!!" << endl;
			keys.clear();
			vals.clear();
		}
		
		// Double check that all key/value strings are not empty so cMsg doesn't choke
		for(unsigned int i=0; i<keys.size(); i++){
			if(keys[i] == "") keys[i] = "<unknown>";
			if(vals[i] == "") vals[i] = "<empty>";
		}

		// Add all key/value pairs to response
		response.setText("host info");
		if(keys.size() > 0){
			response.add("keys" , keys);
			response.add("vals", vals);
		}

		cMsgSys->send(&response);

		delete msg;
		return;		
	}
	//======================================================================
	if(cmd.find("command line")==0){

		// Add all key/value pairs to response
		response.setText("command line");

		vector<string> args = japp->GetArgs();
		stringstream ss;
		for(unsigned int i=0; i<args.size(); i++) ss << args[i] << " ";
		response.add("command" , ss.str());

		cMsgSys->send(&response);

		delete msg;
		return;		
	}

	delete msg;
}

//---------------------------------
// HostInfoPROC  (Linux)
//---------------------------------
void janactl_plugin::HostInfoPROC(vector<string> &keys, vector<string> &vals)
{
	/// Get host info using the /proc mechanism on Linux machines.
#ifdef HAVE_PROC

#endif // HAVE_PROC
}

//---------------------------------
// HostInfoSYSCTL  (Mac OS X)
//---------------------------------
void janactl_plugin::HostInfoSYSCTL(vector<string> &keys, vector<string> &vals)
{
	/// Get host info using the sysctl mechanism on BSD machines.
	/// (mainly for Mac OS X). To see a list of all available items
	/// on a system, use the sysctl command line utility:
	///
	///  sysctl -A
	///

#ifdef HAVE_SYSCTL
	stringstream ss;
	char buff[256];
	size_t bufflen=256;

	// Model
	bufflen=256;
	bzero(buff, bufflen);
	sysctlbyname("hw.model", buff, &bufflen, NULL, 0);
	keys.push_back("Model");
	vals.push_back(buff);

	// Machine arch
	bufflen=256;
	bzero(buff, bufflen);
	if(sysctlbyname("machdep.cpu.brand_string", buff, &bufflen, NULL, 0) == 0){
		keys.push_back("CPU Brand");
		vals.push_back(buff);
	}

	// Machine type
	bufflen=256;
	bzero(buff, bufflen);
	sysctlbyname("hw.machine", buff, &bufflen, NULL, 0);
	keys.push_back("Machine type");
	vals.push_back(buff);

	// Clock Speed
	uint64_t cpufrequency = 0;
	bufflen=sizeof(cpufrequency);
	sysctlbyname("hw.cpufrequency", &cpufrequency, &bufflen, NULL, 0);
	double fcpufrequency = ((double)cpufrequency)/1.0E9;
	ss.str("");
	ss << fcpufrequency << " GHz";
	keys.push_back("Clock Speed");
	vals.push_back(ss.str());

	// RAM
	uint64_t memsize = 0;
	bufflen=sizeof(memsize);
	sysctlbyname("hw.memsize", &memsize, &bufflen, NULL, 0);
	double fmemsize = ((double)memsize)/1024.0/1024.0/1024.0;
	ss.str("");
	ss << fmemsize << " GB";
	keys.push_back("RAM");
	vals.push_back(ss.str());

	// Ncores (physical)
	uint32_t Ncores = 0;
	bufflen=sizeof(Ncores);
	sysctlbyname("hw.physicalcpu", &Ncores, &bufflen, NULL, 0);
	ss.str("");
	ss << Ncores;
	keys.push_back("Ncores (physical)");
	vals.push_back(ss.str());

	// Ncores (logical)
	Ncores = 0;
	bufflen=sizeof(Ncores);
	sysctlbyname("hw.logicalcpu", &Ncores, &bufflen, NULL, 0);
	ss.str("");
	ss << Ncores;
	keys.push_back("Ncores (logical)");
	vals.push_back(ss.str());
#endif // HAVE_SYSCTL
}

#endif // HAVE_CMSG

