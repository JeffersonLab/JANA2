
#include <unistd.h>
#include <pthread.h>
#include <TGApplication.h>

#include "RootSpy.h"
#include "rs_mainframe.h"
#include "rs_cmsg.h"
#include "rs_info.h"

// GLOBALS
rs_mainframe *RSMF = NULL;
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

//-------------------
// main
//-------------------
int main(int narg, char *argv[])
{
	// Parse the command line arguments
	ParseCommandLineArguments(narg, argv);
	
	// Create rs_info object
	RS_INFO = new rs_info();
	
	// Create a ROOT TApplication object
	TApplication app("RootSpy", &narg, argv);

	// Create cMsg object
	RS_CMSG = new rs_cmsg();

	// Create the GUI window
	RSMF = new rs_mainframe(gClient->GetRoot(), 10, 10);
	
	// Hand control to the ROOT "event" loop
	app.SetReturnFromRun(true);
	app.Run();
	
	delete RS_CMSG;
	delete RSMF;

	return 0;
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{

	for(int i=1;i<narg;i++){
		if(argv[i][0] != '-')continue;
		switch(argv[i][1]){
			case 'h':
				Usage();
				break;
		}
	}
}

//-----------
// Usage
//-----------
void Usage(void)
{
	cout<<"Usage:"<<endl;
	cout<<"       RootSpy [options]"<<endl;
	cout<<endl;
	cout<<"Connect to programs with the rootspy plugin attached"<<endl;
	cout<<"and spy on thier histograms."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h        Print this message"<<endl;
	cout<<endl;

	exit(0);
}
