// Author: David Lawrence  Sat Dec 26 22:25:30 EST 2009
//
//
// janactl.cc
//

#include <sys/time.h>

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
#include "jc_cmsg.h"

string ParseCommandLineArguments(int narg, char *argv[]);
void Usage(void);

string UDL = "cMsg://localhost/cMsg/janactl";
string NAME = "janactl";
string DESCRIPTION = "Access JANA processes remotely";
string SUBJECT = "janactl";
double TIMEOUT = 0.75; // seconds

//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	// Start high resolution timer (if it's not already started)
	struct itimerval start_tmr;
	getitimer(ITIMER_REAL, &start_tmr);	
	if(start_tmr.it_value.tv_sec==0 && start_tmr.it_value.tv_usec==0){
		struct itimerval value, ovalue;
		value.it_interval.tv_sec = 1000000;
		value.it_interval.tv_usec = 0;
		value.it_value.tv_sec = 1000000;
		value.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &value, &ovalue);
	}

	// Parse the command line
	string cmd = ParseCommandLineArguments(narg, argv);

#if HAVE_CMSG
	// Create jc_cmsg object
	jc_cmsg jc(UDL, NAME, DESCRIPTION);
	if(!jc.IsConnected())return -1;
	
	// Set the timeout for the jc_cmsg object
	jc.SetTimeout(TIMEOUT);

	// Handle command
	if(cmd=="list"){
		jc.ListRemoteProcesses();
	}else if(cmd=="thinfo"){
		jc.GetThreadInfo(SUBJECT);
	}else if(cmd=="pause" || cmd=="resume" || cmd=="quit" || cmd=="kill"){
		jc.SendCommand(cmd, SUBJECT);
	}else if(cmd.find("killthread ")==0){
		jc.SendCommand(cmd, SUBJECT);
	}else if(cmd.find("set ")==0){
		jc.SendCommand(cmd, SUBJECT);
	}else if(cmd==("list parms") || cmd=="list params" || cmd.find("list conf")==0){
		jc.ListConfigurationParameters(SUBJECT);
	}else if(cmd=="list sources" || cmd=="sources"){
		jc.ListSources(SUBJECT);
	}else if(cmd.find("source types")==0){
		jc.ListSourceTypes(SUBJECT);
	}else if(cmd.find("factories")==0){
		jc.ListFactories(SUBJECT);
	}else if(cmd.find("plugins")==0){
		jc.ListPlugins(SUBJECT);
	}else if(cmd.find("attach plugin")==0){
		jc.SendCommand(cmd, SUBJECT);
	}else if(cmd=="command" || cmd=="command line"){
		jc.GetCommandLine(SUBJECT);
	}else if(cmd=="hostinfo" || cmd=="host info"){
		jc.GetHostInfo(SUBJECT);
	}else{
		jerr<<"Unknown command \""<<cmd<<"\"!"<<endl;
	}

#else
	jerr<<endl;
	jerr<<" cMsg support has not been compiled into the JANA executables"<<endl;
	jerr<<" you are using. Please reconfigure JANA with the --with-cmsg"<<endl;
	jerr<<" option or the CMSGROOT environment variable set and recompile."<<endl;
	jerr<<endl;
#endif // HAVE_CMSG
	
	return 0;
}

//-----------
// ParseCommandLineArguments
//-----------
string ParseCommandLineArguments(int narg, char *argv[])
{
	if(narg==1)Usage();

	// Handle switches and find the command

	// Loop over command line arguments
	string cmd("");
	bool udl_specified = false;
	for(int i=1;i<narg;i++){
		if(argv[i][0] == '-'){
			// Handle switches
			string arg(&argv[i][1]);
			string arg_next((i+1)<narg ? argv[i+1]:"");
			if(arg=="h" || arg=="help" || arg=="-help")Usage();
			if(arg=="t" || arg=="-timeout"){TIMEOUT = atof(arg_next.c_str()); i++; continue;}
			if(arg=="u" || arg=="-udl"){UDL = arg_next; udl_specified=true; i++; continue;}
			if(arg=="n" || arg=="-name"){NAME = arg_next; i++; continue;}
			if(arg=="d" || arg=="-description"){DESCRIPTION = arg_next; i++; continue;}
			if(arg=="s" || arg=="-subject"){SUBJECT = arg_next; i++; continue;}
		}else{
			// This must be part of the command
			if(cmd.length()>0)cmd += " ";
				
			cmd += argv[i];
		
		} // argv[i][0] == '-'
	}
	
	// If the UDL was not explicitly specified, then look at environment
	if(!udl_specified){
		const char *udl = getenv("JANACTL_UDL");
		if(udl)UDL = udl;
	}
	
	return cmd;
}

//-----------
// Usage
//-----------
void Usage(void)
{
	cout<<"Usage:"<<endl;
	cout<<"       janactl [options] cmd"<<endl;
	cout<<endl;
	cout<<"Communicate with remote or local processes that have the janactl"<<endl;
	cout<<"plugin attached."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h, --help  Print this message"<<endl;
	cout<<"   -t timeout  Set the timeout of commands while waiting for a response."<<endl;
	cout<<"   -u udl      Set UDL of cMsg server. If not given, the JANACTL_UDL environment"<<endl;
	cout<<"               variable is used. If that's not set, the localhost is used."<<endl;
	cout<<"   -n name     Set name of this program for use by cMsg server."<<endl;
	cout<<"   -d descr.   Set description text of this program for use by cMsg server."<<endl;
	cout<<"   -s subject  Send command to subject (default is all \"janactl\" which is"<<endl;
	cout<<"               all processes.)"<<endl;
	cout<<endl;
	cout<<"   --timeout timeout      Same as -t"<<endl;
	cout<<"   --udl udl              Same as -u"<<endl;
	cout<<"   --name name            Same as -n"<<endl;
	cout<<"   --description descr.   Same as -d"<<endl;
	cout<<"   --subject subject      Same as -s"<<endl;
	cout<<endl;
	cout<<"The janactl system uses a passive communication mechansim for control. Specifically,"<<endl;
	cout<<"messages are sent, but responses (if any) are received asynchronously. As such"<<endl;
	cout<<"commands like the \"list\" command simply broadcast a query to all processes"<<endl;
	cout<<"that asks for them to send back a message notifying us of their existence. At "<<endl;
	cout<<"some point, we must decide all responses have been received and continue on."<<endl;
	cout<<"the \"timeout\" for this is set by default to 0.75 seconds, but an alternate may be set"<<endl;
	cout<<"via the \"-t timeout\" command line switch. Commands that expect only a single"<<endl;
	cout<<"response will continue as soon as that response is recieved. To target a query to"<<endl;
	cout<<"a specific process, use the \"-s subject\" option. (Hint: run \"janactl list\" to get"<<endl;
	cout<<"a list of valid strings for the \"subject\" argument.)"<<endl;
	cout<<endl;
	cout<<" commands:"<<endl;
	cout<<"   list                Lists available processes."<<endl;
	cout<<"   pause               Pause remote process(es)"<<endl;
	cout<<"   resume              Resume remote process(es)"<<endl;
	cout<<"   quit                Quit remote process(es) gracefully"<<endl;
	cout<<"   kill                Kill remote process(es) harshly"<<endl;
	cout<<"   thinfo              Get thread info. for remote process(es)"<<endl;
	cout<<"   killthread thread   Kill specified thread"<<endl;
	cout<<"   set nthreads N      Change number of processing threads to N"<<endl;
	cout<<"   list parms          Lists configuration parameters (only first response received.)"<<endl;
	cout<<"   list sources        Lists event sources for remote process(es)"<<endl;

	cout<<"   list source types   Lists supported event source types for remote process(es)"<<endl;
	cout<<"   list factories      Lists factories for remote process(es)"<<endl;
	cout<<"   list plugins        Lists attached plugins for remote process(es)"<<endl;
	cout<<"   attach plugin X     Have remote process(es) attach plugin X"<<endl;
	cout<<"                       n.b. X must exist on the remote machine and be"<<endl;
	cout<<"                       in the JANA_PLUGIN_PATH"<<endl;
	cout<<"   command line        Print command line used to start remote process(es)"<<endl;
	cout<<"   host info           Print host info (CPU, RAM, ...) for remote process(es)"<<endl;
	cout<<endl;

	exit(0);
}


