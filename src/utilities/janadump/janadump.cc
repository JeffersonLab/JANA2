// Author: David Lawrence  June 25, 2004
//
//
// hd_ana.cc
//

#include <iostream>
using namespace std;

#include <termios.h>

#include <JANA/JApplication.h>

#include "MyProcessor.h"

void PrintFactoryList(JApplication *app);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

bool LIST_FACTORIES = false;
bool SPARSIFY_SUMMARY = true;

//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	// Parse the command line
	ParseCommandLineArguments(narg, argv);

	// Instantiate our event processor
	MyProcessor myproc;

	// Instantiate an event loop object
	JApplication *app = new JApplication(narg, argv);
	
	// Set tag prefix for JANA streams to empty
	jout.SetTag("");
	
	// If LIST_FACTORIES is set, print all factories and exit
	if(LIST_FACTORIES){
		PrintFactoryList(app);
		return 0;
	}

	// This monkey shines is needed to get getchar() to return single
	// characters without waiting for the user to hit return
	struct termios t;
	tcgetattr(fileno(stdin), &t);
	t.c_lflag &= (~ICANON);
	//t.c_cc[VMIN] = 1;
	tcsetattr(fileno(stdin), TCSANOW, &t);


	// Run though all events, calling our event processor's methods
	app->SetShowTicker(0);
	app->monitor_heartbeat = false;
	app->Run(&myproc);
	
	int exit_code = app->GetExitCode();
	
	delete app;

	return exit_code;
}

//-----------
// PrintFactoryList
//-----------
void PrintFactoryList(JApplication *app)
{
	// When we get here, the Run() method hasn't been
	// called so the JEventLoop objects haven't
	// been created yet and consequently the factory objects
	// don't yet exist. Since we want the "list factories"
	// option to work even without an input file, we need
	// to first make the factories before we can list them.
	// To do this we only need to instantiate a JEventLoop object
	// passing it our "app" pointer. The JEventLoop will automatically
	// register itself with the JApplication and the factories
	// will be made, even ones from plugins passed on the command
	// line.
	app->Init();
	JEventLoop *loop = new JEventLoop(app);
	
	// Print header
	cout<<endl;
	cout<<"  Factory List"<<endl;
	cout<<"-------------------------"<<endl;
	
	// Get list of factories from the JEventLoop and loop over them
	// Printing out the data types and tags.
	vector<JFactory_base*> factories = loop->GetFactories();
	vector<JFactory_base*>::iterator iter = factories.begin();
	for(; iter!=factories.end(); iter++){
		cout<<" "<<(*iter)->GetDataClassName();
		if(strlen((*iter)->Tag()) !=0){
			cout<<" : "<<(*iter)->Tag();
		}
		cout<<endl;
	}
	cout<<endl;
	cout<<" "<<factories.size()<<" factories registered"<<endl;
	cout<<endl;
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(narg==1)Usage();

	for(int i=1;i<narg;i++){
		if(argv[i][0] != '-')continue;
		switch(argv[i][1]){
			case 'h':
				Usage();
				break;
			case 'D':
				toprint.push_back(&argv[i][2]);
				break;
			case 'p':
				PAUSE_BETWEEN_EVENTS = 0;
				break;
			case 's':
				SKIP_BORING_EVENTS = 1;
				break;
			case 'A':
				PRINT_ALL = 1;
				break;
			case 'L':
				LIST_FACTORIES = 1;
			case 'a':
				LIST_ASSOCIATED_OBJECTS = true;
				break;
			case 'S':
				SPARSIFY_SUMMARY = false;
				break;
			case 't':
				ACTIVATE_TAGGED_FOR_SUMMARY = true;
				break;
			case 'f':
				PRINT_SUMMARY_HEADER = false;
				break;
			case 'b':
				PRINT_STATUS_BITS = true;
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
	cout<<"       janadump [options] source1 source2 ..."<<endl;
	cout<<endl;
	cout<<"Print the contents of a JANA data source (e.g. a file)"<<endl;
	cout<<"to the screen."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h        Print this message"<<endl;
	cout<<"   -Dname    Print the data of type \"name\" (can be used multiple times)"<<endl;
	cout<<"   -A        Print ALL data types (overrides and -DXXX options)"<<endl;
	cout<<"   -L        List available factories and exit"<<endl;
	cout<<"   -p        Don't pause for keystroke between events (def. is to pause)"<<endl;
	cout<<"   -s        Skip events which don't have any of the specified data types"<<endl;
	cout<<"   -a        List types and number of associated objects"<<endl;
	cout<<"   -S        Don't supress printing of factories with no objects in summary"<<endl;
	cout<<"   -t        Activate factories with non-empty tags when printing summary"<<endl;
	cout<<"   -f        Skip printing summary header lisiting all factories"<<endl;
	cout<<"             (This disables auto-activating every single factory)"<<endl;
	cout<<"   -b        Print event status bits"<<endl;
	cout<<endl;

	exit(0);
}


