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
double TIMEOUT = 3.0; // seconds

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
	string cmd;
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
			// This must be the command
			cmd = argv[i];
		
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
	cout<<"   -d descr.   Set description text of this program for use by cMsg server."<<endl;
	cout<<"   -s subject  Send command to subject (default is all \"janactl\" which is"<<endl;
	cout<<"               all processes."<<endl;
	cout<<endl;
	cout<<"   --timeout timeout      Same as -t"<<endl;
	cout<<"   --udl udl              Same as -u"<<endl;
	cout<<"   --name name            Same as -n"<<endl;
	cout<<"   --description descr.   Same as -d"<<endl;
	cout<<"   --subject subject      Same as -s"<<endl;
	cout<<endl;
	cout<<"The janactl system uses a passive comminucation mechansim for control. Specifically,"<<endl;
	cout<<"messages are sent, but responses (if any) are received asynchronously. As such"<<endl;
	cout<<"commands such as the \"list\" command simply broadcasts a query to all processes"<<endl;
	cout<<"that askes for them to send back a message notifying us of their existence. At "<<endl;
	cout<<"some point, we must decide all responses have been received and continue on."<<endl;
	cout<<"the \"timeout\" for this is set by default to 3 seconds, but an alternate may be set"<<endl;
	cout<<"via the \"-t timeout\" command line switch. Commands that expect only a single"<<endl;
	cout<<"response will continue as soon as that response is recieved."<<endl;
	cout<<endl;
	cout<<" commands:"<<endl;
	cout<<"   list        Lists available processes."<<endl;
	cout<<"   pause       Pause remote process(es)"<<endl;
	cout<<"   resume      Resume remote process(es)"<<endl;
	cout<<"   quit        Quit remote process(es) gracefully"<<endl;
	cout<<"   kill        Kill remote process(es) harshly"<<endl;
	cout<<"   thinfo      Get thread info. for remote process(es)"<<endl;
	cout<<endl;

	exit(0);
}


