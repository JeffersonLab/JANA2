// Author: David Lawrence   DEc. 17, 2013
//
//
// jresource.cc
//

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
using namespace std;

#include <stdlib.h>
#include <unistd.h>

#include <JANA/JApplication.h>
#include <JANA/JCalibration.h>
#include <JANA/md5.h>
using namespace jana;

template<class T> void PrintDataT(JCalibration *jcalib);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);
string Get_MD5(string fullpath);

unsigned int RUN_NUMBER = 100;
bool LIST_LOCAL_RESOURCES = false;
string GET_RESOURCE_NAMEPATH="";
string INFO_RESOURCE_NAMEPATH="";
string REMOVE_RESOURCE_NAMEPATH="";
string ADD_RESOURCE_NAMEPATH="";
string ADD_RESOURCE_RUNRANGE="";
string ADD_RESOURCE_URL_BASE="";
string ADD_RESOURCE_PATH="";
string ADD_RESOURCE_VARIATION="default";


//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	// Parse the command line
	ParseCommandLineArguments(narg, argv);

	// Instantiate a JApplication object and use it to get a JCalibration object
	JApplication *app = new JApplication(narg, argv);
	app->create_event_buffer_thread = false;
	app->Init();
	JCalibration *jcalib = app->GetJCalibration(RUN_NUMBER);

	// Make sure the calibration object exists
	if(jcalib==NULL)return -1;

	// Use JApplication to create a JResourceManager
	JResourceManager *resman = app->GetJResourceManager();
	
	// If LIST_NAMEPATHS is true then list the namepaths
	if(LIST_LOCAL_RESOURCES){
		map<string,string> resources = resman->GetLocalResources();
		string resource_dir = resman->GetLocalPathToResource("");

		cout<<endl;
		cout<<"   Local Resources"<<endl;
		cout<<"   (from " << resource_dir << ")" << endl;
		cout<<"--------------------"<<endl;

		map<string,string>::iterator iter;
		for(iter=resources.begin(); iter!=resources.end(); iter++){
			cout << "namepath: " << iter->second << endl;
			cout << "URL: " << iter->first << endl;
			cout << "full path: " << resource_dir << iter->second << endl;
			cout << endl;
		}
	}
	
	// if GET_RESOURCE_NAMEPATH is not an empty string, try getting the resource
	if(GET_RESOURCE_NAMEPATH != ""){

		// Calculate MD5 checksum of existing file (if any)
		string fullpath = resman->GetLocalPathToResource(GET_RESOURCE_NAMEPATH);
		string md5_before = Get_MD5(fullpath);

		try{
			string fp = resman->GetResource(GET_RESOURCE_NAMEPATH);
			string md5 = Get_MD5(fp);
			
			cout << endl;

			if(md5_before.length() > 0){
				if(md5_before != md5){
					cout << "==== modified resouce ====" << endl;
					cout << "before: md5=" << md5_before << " path=" << fullpath << endl;
					cout << " after: md5=" << md5 << " path=" << fp << endl;
				}else{
					cout << "==== resource already exists locally ====" << endl;
					cout << "md5=" << md5 << " path=" << fp << endl;
				}
			}else{
				cout << "==== retrieved resource ====" << endl;
				cout << "md5=" << md5 << " path=" << fp << endl;
			}
			
		}catch(JException &exception){
			cout << exception.toString(false) << endl;
		}
	}

	// if REMOVE_RESOURCE_NAMEPATH is not an empty string, try deleting the local file
	if(REMOVE_RESOURCE_NAMEPATH != ""){
		
		// check if file exists
		string fullpath = resman->GetLocalPathToResource(REMOVE_RESOURCE_NAMEPATH);
		ifstream myf(fullpath.c_str());
		cout << endl;
		if(myf){
			cout << "Deleting file: " << fullpath << endl;
			myf.close();
			unlink(fullpath.c_str());
		}else{
			cout << "File \""<<fullpath<<"\" doesn't exist!" << endl;
		}
	}

	// if INFO_RESOURCE_NAMEPATH is not an empty string, print info about the resource
	if(INFO_RESOURCE_NAMEPATH != ""){
		string fullpath = resman->GetLocalPathToResource(INFO_RESOURCE_NAMEPATH);
		string md5 = Get_MD5(fullpath);
		if(md5 == "") md5 = "<N/A>";
		
		map<string, string> info;
		jcalib->Get(INFO_RESOURCE_NAMEPATH, info);
		
		unsigned int fsize = 0;
		ifstream ifs(fullpath.c_str());
		if(ifs.is_open()){
			ifs.seekg(0, ifs.end);
			fsize = ifs.tellg();
			ifs.close();
		}

		cout << endl;
		cout << "===================================================" << endl;
		cout << "DB location: " << jcalib->GetURL() << endl;
		cout << "   namepath: " << INFO_RESOURCE_NAMEPATH << endl;
		cout << "   URL_base: " << info["URL_base"] << endl;
		cout << "       path: " << info["path"] << endl;
		cout << "        run: " << jcalib->GetRun() << endl;
		cout << " local path: " << fullpath << endl;
		cout << "        md5: " << md5 << endl;
		cout << "  file size: " << fsize << " bytes (" << fsize/1024/1024 << " MB)" << endl;
		cout << "===================================================" << endl;
	}

	// if ADD_RESOURCE_NAMEPATH is not an empty string, try adding a resource
	if(ADD_RESOURCE_NAMEPATH != ""){
	
		// Only do this for CCDB
		if(jcalib->className() != string("JCalibrationCCDB")){
			cout << "ERROR!! The add function can only be used with CCDB as the" << endl;
			cout << "calibration database backend! Aborting!" <<endl;
			exit(-1);
		}
		
		// Create temporary file with resource info
		ofstream ofs(".jresource_tmp");
		if(!ofs.is_open()){
			cout << "Unable to create temporary file! Make sure you are running" << endl;
			cout << "this from a directory where you have write permissions!" << endl;
			exit(-2);
		}
		
		// Download the file to get the md5 checksum
		string URL = ADD_RESOURCE_URL_BASE + "/" + ADD_RESOURCE_PATH;
		string tmpfile = ".jresource_tmp_data_file";
		try{
			cout<<"Testing resource URL by downloading resource:"<<endl;
			resman->GetResourceFromURL(URL, tmpfile);
		}catch(JException &e){
			cerr<<"--- Error getting resource ---"<<endl;
			cerr<< e.toString() << endl;
			ofs.close();
			unlink(".jresource_tmp");
			exit(-1);
		}
		string md5 = Get_MD5(tmpfile);
		
		// Write resource info into temporary file
		ofs << "URL_base " << ADD_RESOURCE_URL_BASE << endl;
		ofs << "path     " << ADD_RESOURCE_PATH << endl;
		ofs << "md5      " << md5 << endl;
		ofs.close();
		
		// We need to make sure the "directory" for the desired namepath exists
		// in the CCDB so peel off the directory part.
		string namepath_dir = "";
		size_t pos = ADD_RESOURCE_NAMEPATH.find_last_of("/");
		if(pos != string::npos ){
			namepath_dir = ADD_RESOURCE_NAMEPATH.substr(0,pos);
		}
		
		// Create ccdb commands
		vector<string> cmds;
		if(namepath_dir != "") cmds.push_back("ccdb mkdir " + namepath_dir);
		cmds.push_back("ccdb mktbl " + ADD_RESOURCE_NAMEPATH + " URL_base=string path=string md5=string");
		cmds.push_back("ccdb add --name-value " + ADD_RESOURCE_NAMEPATH + " -v " + ADD_RESOURCE_VARIATION + " -r " + ADD_RESOURCE_RUNRANGE + " .jresource_tmp");

		// The ccdb commands will use the CCDB_CONNECTION environment variable
		// while JANA's jcalib uses JANA_CALIB_URL. Explicitly set the CCDB_CONNECTION
		// environment variable to be the URL jcalib is using so that the commands
		// issued will act on the correct DB.
		setenv("CCDB_CONNECTION", jcalib->GetURL().c_str(), 1);

		// Tell user what we're about to do
		cout << endl;
		cout << "==== Add resource to CCDB ====" << endl;
		cout << "DB location: " << jcalib->GetURL() << endl;
		cout << "   namepath: " << ADD_RESOURCE_NAMEPATH << endl;
		cout << "  variation: " << ADD_RESOURCE_VARIATION << endl;
		cout << "   URL_base: " << ADD_RESOURCE_URL_BASE << endl;
		cout << "       path: " << ADD_RESOURCE_PATH << endl;
		cout << "  run range: " << ADD_RESOURCE_RUNRANGE << endl;
		cout << "        md5: " << md5 << endl;
		cout << endl;
		cout << "----- CCDB commands to be executed -----" << endl;
		for(unsigned int i=0; i<cmds.size(); i++) cout << i+1 << ".) " << cmds[i] << endl;
		cout << endl;
		cout << "Type \"Y\" then ENTER to continue. Any other key then ENTER to abort: "; cout.flush();
		string s;
		cin >> s;
		if(s=="Y" || s=="y"){
			// Execute command
			cout << endl;
			cout << "NOTE: The mkdir and mktbl commands may produce errors if the" << endl;
			cout << "structure already exists in the CCDB. These errors can be safely" <<endl;
			cout << "ignored. The final command will then just add a newer entry." << endl;
			cout << endl;
			cout << "Executing commands ... " << endl;
			for(unsigned int i=0; i<cmds.size(); i++){
				cout << "---------------------------------------------------------" << endl;
				cout << i+1 << ".) " << cmds[i] << endl;
				cout << endl;
				system(cmds[i].c_str());
			}
			cout << "---------------------------------------------------------" << endl;
			cout << endl;
			cout << "You can now try retrieving the resource with:" <<endl;
			cout << endl;
			cout << "  jresource -g " << ADD_RESOURCE_NAMEPATH << endl;
			cout << endl;
		}else{
			cout << "Aborted." << endl;
		}
		
		// Delete temporary file
		unlink(".jresource_tmp");
		unlink(tmpfile.c_str());
	}
	
	int exit_code = app->GetExitCode();

	delete app;

	cout << endl;

	return exit_code;
}

//-----------
// Get_MD5
//-----------
string Get_MD5(string fullpath)
{
	ifstream ifs(fullpath.c_str());
	if(!ifs.is_open()) return string("");

	md5_state_t pms;
	md5_init(&pms);
	
	// allocate 10kB buffer for reading in file
	unsigned int buffer_size = 10000; 
	char *buff = new char [buffer_size];

	// read data as a block:
	while(ifs.good()){
		ifs.read(buff,buffer_size);
		md5_append(&pms, (const md5_byte_t *)buff, ifs.gcount());
	}
	ifs.close();

	delete[] buff;

	md5_byte_t digest[16];
	md5_finish(&pms, digest);
	
	char hex_output[16*2 + 1];
	for(int di = 0; di < 16; ++di) sprintf(hex_output + di * 2, "%02x", digest[di]);

	return string(hex_output);
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(narg==1)Usage();

	for(int i=1;i<narg;i++){
		if(argv[i][0] == '-'){
			string arg = "";
			if(i+1 < narg) arg  = argv[i+1];
			switch(argv[i][1]){
				case 'h':
					Usage();
					break;
				case 'R':
					if(arg==""){cout<<"'"<<argv[i][1]<<"' requires an argument!"<<endl; exit(0);}
					RUN_NUMBER = atoi(arg.c_str());
					break;
				case 'L':
					LIST_LOCAL_RESOURCES = true;
					break;
				case 'g':
					if(arg==""){cout<<"'"<<argv[i][1]<<"' requires an argument!"<<endl; exit(0);}
					GET_RESOURCE_NAMEPATH = arg;
					break;
				case 'i':
					if(arg==""){cout<<"'"<<argv[i][1]<<"' requires an argument!"<<endl; exit(0);}
					INFO_RESOURCE_NAMEPATH = arg;
					break;
				case 'r':
					if(arg==""){cout<<"'"<<argv[i][1]<<"' requires an argument!"<<endl; exit(0);}
					REMOVE_RESOURCE_NAMEPATH = arg;
					break;
				case 'a':
					if(narg <= i+3){cout<<"'"<<argv[i][1]<<"' requires at least 3 arguments!"<<endl; exit(0);}
					ADD_RESOURCE_NAMEPATH = arg;
					ADD_RESOURCE_RUNRANGE = argv[i+2];
					ADD_RESOURCE_URL_BASE = argv[i+3];
					if(narg > i+4) ADD_RESOURCE_PATH = argv[i+4];
					if(narg > i+5) ADD_RESOURCE_VARIATION = argv[i+5];
					if(ADD_RESOURCE_PATH == "") ADD_RESOURCE_PATH = ADD_RESOURCE_NAMEPATH;
					break;
			}
		}
	}
}

//-----------
// Usage
//-----------
void Usage(void)
{
	// Make sure a JApplication exists
	if(!japp){
		char* argv[0];
		new JApplication(0, argv);
	}

	// Print Usage statement
	cout<<"Usage:"<<endl;
	cout<<"       jresource [options]"<<endl;
	cout<<endl;
	cout<<"JANA resource utility"<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h              Print this message"<<endl;
	cout<<"   -R run          Set run number to \"run\""<<endl;
	cout<<"   -L              List local resources"<<endl;
	cout<<"   -g namepath     Get the specified resource"<<endl;
	cout<<"   -i namepath     Print info on specified resource"<<endl;
	cout<<"   -r namepath     remove resource (local file only)"<<endl;
	cout<<"   -a namepath ... add resource to calibration DB (CCDB only)"<<endl;
	cout<<"                   (see example below for full command)"<<endl;
	cout<<endl;
	cout<<" The \"-a\" option can be used to add a new resource to a CCDB"<<endl;
	cout<<"calibration database by issuing it with the following form:"<<endl;
	cout<<endl;
	cout<<"   jresource -a namepath  runrange  URL_base  [path  [variation]]"<<endl;
	cout<<endl;
	cout<<" If the \"path\" parameter is not given, then it defaults to the value"<<endl;
	cout<<"given for \"namepath\" (this is what shouyld be done for GlueX.). If no"<<endl;
	cout<<"value is given for \"variation\" then \"default\" is used."<<endl;
	cout<<endl;
	cout<<" For example:"<<endl;
	cout<<endl;
	cout<<"   jresource -a Magnets/Solenoid/solenoid_1350_poisson_20130925 0- https://halldweb1.jlab.org/resources"<<endl;
	cout<<endl;
	cout<<" This will create an entry in the CCDB with the given namepath which covers"<<endl;
	cout<<"all run numbers (\"0-\" means run zero to infinity) and with the full URL of:"<<endl;
	cout<<endl;
	cout<<" https://halldweb1.jlab.org/resources/Magnets/Solenoid/solenoid_1350_poisson_20130925"<<endl;
	cout<<endl;
	cout<<" This works by creating a temporary text file and running the \"ccdb\""<<endl;
	cout<<"command with appropriate arguments in a subshell. It assumes that ccdb"<<endl;
	cout<<"is in your PATH and JANA_CALIB_URL is set properly. (Note that the"<<endl;
	cout<<"CCDB_CONNECTION environment variable is replaced with whatever"<<endl;
	cout <<"JANA_CALIB_URL is set to before running ccdb). The arguments are:"<<endl;
	cout<<endl;
	cout<<"   namepath   namepath corresponding to resource in CCDB"<<endl;
	cout<<"   runrange   run range in CCDB format (i.e. 0- means all runs)"<<endl;
	cout<<"   URL_base   beginning part of URL "<<endl;
	cout<<"   path       ending part of URL and path of file relative to resource dir."<<endl;
	cout<<"              (this will be set to namepath if omitted)"<<endl;
	cout<<"   variation  (optional) if omitted, \"default\" is used"<<endl;
	cout<<endl;
	cout<<" The full URL to the resource is made by combining the URL_base and"<<endl;
	cout<<"path arguments."<<endl;
	cout<<endl;
	cout<<"One should also note that the \"-a\" option is the only command"<<endl;
	cout<<"that will ask for confirmation before execution. The ccdb commands"<<endl;
	cout<<"that will be executed will be printed first so you can see what is"<<endl;
	cout<<"about to be done before it actually happens. Confirmation is requested"<<endl;
	cout<<"because it is the only command that can modify the CCDB."<<endl;
	cout<<endl;
	cout<<endl;

	exit(0);
}


