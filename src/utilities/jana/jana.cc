// Author: Edward Brash February 15, 2005
//
//
// hd_root.cc
//

#include <dlfcn.h>


#include "MyProcessor.h"
#include <JANA/JApplication.h>
#include <JANA/JVersion.h>

using namespace std;

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);



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
	JApplication app(narg, argv);

	// Run though all events, calling our event processor's methods
	app.Run(&myproc);
	
	return app.GetExitCode();
}


//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(narg==1)Usage();

	for(int i=1;i<narg;i++){
		if(argv[i][0] != '-')continue;
		
		string name, tag;
		unsigned int pos;
		switch(argv[i][1]){
			case 'h':
				Usage();
				break;
			case 'v':
				cout<<"          JANA version: "<<JVersion::GetVersion()<<endl;
				cout<<"        JANA ID string: "<<JVersion::GetIDstring()<<endl;
				cout<<"     JANA SVN revision: "<<JVersion::GetSVNrevision()<<endl;
				cout<<"JANA last changed date: "<<JVersion::GetDate()<<endl;
				cout<<"              JANA URL: "<<JVersion::GetURL()<<endl;
				exit(0);
				break;
			case 'D':
				name = &argv[i][2];
				tag = "";
				pos = name.rfind(":",name.size()-1);
				if(pos != (unsigned int)string::npos){
					tag = name.substr(pos+1,name.size());
					name.erase(pos);
				}
				autoactivate[name] = tag;
				break;
			case 'A':
				ACTIVATE_ALL = 1;
				break;
			case '-':
				if(string(argv[i])=="--help"){
					Usage();
					break;
				}
				break;
		}
	}
}


//-----------
// Usage
//-----------
void Usage(void)
{
	// To print the options from JApplication, we need a JApplication object
	extern jana::JApplication *japp;
	if(japp==NULL)new JApplication(0,NULL);

	cout<<"Usage:"<<endl;
	cout<<"       jana [options] source1 source2 ..."<<endl;
	cout<<endl;
	cout<<"Empty jana executable. This can be used with plugins to"<<endl;
	cout<<"read in events and process them. The -D and -A options can"<<endl;
	cout<<"be used to activate factories every event, regardless of"<<endl;
	cout<<"whether they are asked for by a JEventProcessor or JFactory."<<endl;
	cout<<"To specify a tagged factory, append a colon and then the tag"<<endl;
	cout<<"to the data class. e.g.  dataclassname:tag"<<endl;
	cout<<endl;
	cout<<"Base JANA options:"<<endl;
	cout<<endl;
	if(japp)japp->Usage();
	cout<<endl;
	cout<<"Options for jana utility:"<<endl;
	cout<<endl;
	cout<<"   -h       Print this message"<<endl;
	cout<<"   --help   Print this message"<<endl;
	cout<<"   -v       Print the JANA version number"<<endl;
	//cout<<"   -Dname   Activate factory for data of type \"name\" (can be used mult. times)"<<endl;
	cout<<"   -A       Activate all factories"<<endl;
	cout<<endl;

	if(japp!=NULL)delete japp;

	exit(0);
}
