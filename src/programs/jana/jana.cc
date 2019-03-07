// Author: Edward Brash February 15, 2005
//
//
// hd_root.cc
//

#include <iostream>
using namespace std;

#include <dlfcn.h>


#include "MyProcessor.h"
#include <JANA/JApplication.h>
#include <JANA/JVersion.h>
#include <JANA/JSignalHandler.h>

using namespace std;

void ParseCommandLineArguments(int &narg, char *argv[],
                               JParameterManager* params,
                               std::vector<string>* eventSources);
void Usage(void);



//-----------
// main
//-----------
int main(int narg, char *argv[])
{

	// Parse the command line
  auto params = new JParameterManager;
  auto eventSources = new std::vector<string>;
	ParseCommandLineArguments(narg, argv, params, eventSources);

	// Instantiate an event loop object
	JApplication app(params, eventSources);

	japp = &app;
	AddSignalHandlers();

	// Run though all events, calling our event processor's methods
	app.Run();

	return app.GetExitCode();
}

//-----------
// ParseCommandLineArguments
//-----------
// Other args we might want:
// --config, --dumpcalibrations, --dumpconfig, --listconfig, --resourcereport

void ParseCommandLineArguments(int &narg, char *argv[],
                               JParameterManager* params,
                               std::vector<string>* eventSources)
{
	if(narg==1) Usage();

	for(int i=1;i<narg;i++){

		if(argv[i][0] != '-') {
      string arg = argv[i];
      eventSources->push_back(arg);
      // TODO: Consider making eventSources be a normal parameter
      continue;
    }

		string name, tag;
		unsigned int pos;
    string arg = argv[i];
		switch(argv[i][1]){

      case 'P':
        pos = arg.find("=");
        if( (pos!= string::npos) && (pos>2) ){
          string key = arg.substr(2, pos-2);
          string val = arg.substr(pos+1);
          params->SetParameter(key, val);
        }else{
          cout << " Bad parameter argument '" << arg
               << "': Expected format -Pkey=value" << endl;
        }
        break;

			case 'h':
				Usage();
				break;

			case 'v':
				cout<<"          JANA version: "<<JVersion::GetVersion()<<endl;
				cout<<"        JANA ID string: "<<JVersion::GetIDstring()<<endl;
				cout<<"     JANA git revision: "<<JVersion::GetRevision()<<endl;
				cout<<"JANA last changed date: "<<JVersion::GetDate()<<endl;
				cout<<"           JANA source: "<<JVersion::GetSource()<<endl;
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
				// autoactivate[name] = tag;
				break;
			case 'A':
//				ACTIVATE_ALL = 1;
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
	extern JApplication *japp;
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
//	if(japp)japp->Usage();
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
