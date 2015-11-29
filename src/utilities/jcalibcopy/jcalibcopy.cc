// Author: David Lawrence  June 25, 2004
//
//
// hd_ana.cc
//

#include <iostream>
using namespace std;

#include <stdlib.h>

#include <JANA/JApplication.h>
#include <JANA/JCalibration.h>
using namespace jana;

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

unsigned int SRC_RUN;
unsigned int DEST_RUN_MIN;
unsigned int DEST_RUN_MAX;
unsigned int DEST_EVENT_MIN=0;
unsigned int DEST_EVENT_MAX=0;
string NAMEPATH="";
string SRC_URL="";
string DEST_URL="";
string SRC_CONTEXT="";
string DEST_CONTEXT="";
bool DATA_IS_TABLE = false;
string AUTHOR = "";
vector<char*> ARGV;

//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	// Parse the command line
	ParseCommandLineArguments(narg, argv);

	// Instantiate a JApplication object and use it to get a JCalibration object
	JApplication *app = new JApplication((int)ARGV.size(), &(ARGV[0]));
	app->Init();
	
	// Get pointer to source URL
	string url = string("JANA_CALIB_URL=")+SRC_URL;
	string context = string("JANA_CALIB_CONTEXT=")+SRC_CONTEXT;
	putenv(strdup(url.c_str()));
	putenv(strdup(context.c_str()));
	JCalibration *jcalib_src = app->GetJCalibration(SRC_RUN);
	if(jcalib_src==NULL){
		_DBG_<<"Unable to calibration object for \""<<SRC_URL<<"\" with context \""<<SRC_CONTEXT<<"\""<<endl;
		return -1;
	}

	// Get pointer to destination URL
	url = string("JANA_CALIB_URL=")+DEST_URL;
	context = string("JANA_CALIB_CONTEXT=")+DEST_CONTEXT;
	putenv(strdup(url.c_str()));
	putenv(strdup(context.c_str()));
	JCalibration *jcalib_dest = app->GetJCalibration(DEST_RUN_MIN);
	if(jcalib_dest==NULL){
		_DBG_<<"Unable to calibration object for \""<<DEST_URL<<"\" with context \""<<DEST_CONTEXT<<"\""<<endl;
		return -1;
	}
	
	// Container type depends on whether data is 1-D or 2-D
	if(DATA_IS_TABLE){
		// Get Constants from source
		vector< map<string, string> > svals;
		jcalib_src->GetCalib(NAMEPATH, svals);
		
		// Make comment
		string comment = "Copied with jcalibcopy from \""+SRC_URL+"\" with context \""+SRC_CONTEXT+"\"";
		
		// Write constants to destination
		jcalib_dest->PutCalib(NAMEPATH, DEST_RUN_MIN, DEST_RUN_MAX, DEST_EVENT_MIN, DEST_EVENT_MAX, AUTHOR, svals, comment);
	}else{
		// Get Constants from source
		map<string, string> svals;
		jcalib_src->GetCalib(NAMEPATH, svals);
		
		// Make comment
		string comment = "Copied with jcalibcopy from \""+SRC_URL+"\" with context \""+SRC_CONTEXT+"\"";
		
		// Write constants to destination
		jcalib_dest->PutCalib(NAMEPATH, DEST_RUN_MIN, DEST_RUN_MAX, DEST_EVENT_MIN, DEST_EVENT_MAX, AUTHOR, svals, comment);
	}

	int exit_code = app->GetExitCode();

	delete app;

	return exit_code;
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(narg==1)Usage();

	// Keep list of arguments to be passed to JApplication constructor
	ARGV.clear();
	ARGV.push_back(argv[0]);
	
	for(int i=1;i<narg;i++){
		string arg(argv[i]);
		string next_arg = (i+1)<narg ? string(argv[i+1]):string("");

		if(arg=="-h"){Usage(); continue;}

		if(arg=="-R"){
			SRC_RUN = atoi(next_arg.c_str());
			i++;
			continue;
		}
		if(arg=="-Rmin"){
			DEST_RUN_MIN = atoi(next_arg.c_str());
			i++;
			continue;
		}
		if(arg=="-Rmax"){
			DEST_RUN_MAX = atoi(next_arg.c_str());
			i++;
			continue;
		}		
		if(arg=="-Emin"){
			DEST_EVENT_MIN = atoi(next_arg.c_str());
			i++;
			continue;
		}
		if(arg=="-Emax"){
			DEST_EVENT_MAX = atoi(next_arg.c_str());
			i++;
			continue;
		}		
		if(arg=="-T"){
			DATA_IS_TABLE = true;
			continue;
		}
		if(arg=="-a"){
			AUTHOR = next_arg;
			i++;
			continue;
		}		

		// Non-switched options
		if(NAMEPATH==""){
			NAMEPATH = arg;
			continue;
		}
		if(SRC_URL==""){
			SRC_URL = arg;
			continue;
		}
		if(DEST_URL==""){
			DEST_URL = arg;
			continue;
		}
		
		// If we get here then this not an option specific to us.
		// Add it to the list of arguments to be passed to JApplication
		ARGV.push_back(argv[i]);
	}
	
	// Default the author to the current user
	if(AUTHOR==""){
		const char *USER = getenv("USER");
		if(!USER)USER="unknown";
		AUTHOR = USER;
	}
	
	// Look to see if a context string was appended to either URL.
	// Separate it here if it was.
	size_t pos = SRC_URL.find("@");
	if(pos!=string::npos){
		SRC_CONTEXT = SRC_URL.substr(pos+1);
		SRC_URL.erase(pos);
	}
	pos = DEST_URL.find("@");
	if(pos!=string::npos){
		DEST_CONTEXT = DEST_URL.substr(pos+1);
		DEST_URL.erase(pos);
	}

}

//-----------
// Usage
//-----------
void Usage(void)
{
	cout<<"Usage:"<<endl;
	cout<<"       jcalibcopy [options] -R src_run -Rmin dest_run_min -Rmax dest_run_max namepath src_url dest_url"<<endl;
	cout<<endl;
	cout<<"Copy item from one JANA data source to another."<<endl;
	cout<<endl;
	cout<<"Required:"<<endl;
	cout<<endl;
	cout<<"   -R src_run            Set run number for source"<<endl;
	cout<<"   -Rmin dest_run_min    Set min run number for destination"<<endl;
	cout<<"   -Rmax dest_run_max    Set max run number for destination"<<endl;
	cout<<endl;
	cout<<" Options:"<<endl;
	cout<<"   -T                    Data set is a 2-D table (def. assume 1-D array)"<<endl;
	cout<<"   -a author             Set author (def. use USER env. var.)"<<endl;
	cout<<"   -Emin dest_event_min  Set min event number for destination(def. 0)"<<endl;
	cout<<"   -Emax dest_event_max  Set max event number for destination(def. 0)"<<endl;
	cout<<"   -h		            Print this message"<<endl;
	cout<<endl;
	cout<<" The url given for either the source or the destination may include a "<<endl;
	cout<<"context by appending \"@context\" to it."<<endl;
	cout<<endl;
	cout<<" e.g."<<endl;
	cout<<"   file:///home/billybob/calib@test1"<<endl;
	cout<<endl;

	exit(0);
}


