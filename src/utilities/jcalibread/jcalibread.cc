// Author: David Lawrence  June 25, 2004
//
//
// hd_ana.cc
//

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#include <stdlib.h>

#include <JANA/JApplication.h>
#include <JANA/JCalibration.h>
using namespace jana;

template<class T> void PrintDataT(JCalibration *jcalib);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

unsigned int RUN_NUMBER = 100;
string NAMEPATH="";
bool DISPLAY_AS_TABLE = false;
bool LIST_NAMEPATHS = false;
string DATATYPE="";

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
	
	// If LIST_NAMEPATHS is true then list the namepaths
	if(LIST_NAMEPATHS){
		vector<string> namepaths;
		jcalib->GetListOfNamepaths(namepaths);
		
		cout<<endl;
		cout<<"Available namepaths:"<<endl;
		cout<<"--------------------"<<endl;
		for(unsigned int i=0; i< namepaths.size(); i++)cout<<namepaths[i]<<endl;
		cout<<endl;
	}
	
	// Get and print constants if NAMEPATH is set
	if(NAMEPATH.length()!=0){
		// Display constants
		cout<<endl<<"Values for \""<<NAMEPATH<<"\" for run "<<RUN_NUMBER<<endl;
		cout<<"--------------------"<<endl;

		// Defaults for when no data type is specified. These were chosen
		// some time ago so I'm not sure if they are still the best option ...
		if(DATATYPE=="")DATATYPE = DISPLAY_AS_TABLE ? "float":"string";

		// We use a templated routine to print the table (see note below)
		if(DATATYPE=="float"){
			PrintDataT<float>(jcalib);
		}else if(DATATYPE=="double"){
			PrintDataT<double>(jcalib);
		}else if(DATATYPE=="string"){
			PrintDataT<string>(jcalib);
		}else if(DATATYPE=="int"){
			PrintDataT<int>(jcalib);
		}else{
			cout<<"Invalid data type \""<<DATATYPE<<"\"!"<<endl;
			cout<<"Valid data types are: float double string int"<<endl;
		}
	
		cout<<endl;
	}

	int exit_code = app->GetExitCode();

	delete app;

	return exit_code;
}

//--------------
// PrintDataT
//--------------
template<class T>
void PrintDataT(JCalibration *jcalib)
{
	/// Templated method for printing data to screen. This used to be 
	/// embedded in main(), but in order to support user specified 
	/// data types, it was moved here allowing cout to decide how to 
	/// display the different types.

	// Oddly enough, this is needed because compiler errors occur
	// when we do a straight map<string,T>::iterator iter;
	typedef typename map<string,T>::iterator iter_T;

	// Table data and key-value data are displayed a little differently
	if(DISPLAY_AS_TABLE){
	
		// DISPLAY DATA AS TABLE
	
		// Get constants
		vector< map<string, T> > tvals;
		jcalib->Get(NAMEPATH, tvals);

		for(unsigned int i=0; i<tvals.size(); i++){
			map<string, T> &row = tvals[i];
			iter_T iter = row.begin();
			for(;iter!=row.end(); iter++){
				cout<<iter->first<<"="<<iter->second<<"  ";
			}
			cout<<endl;
		}
	}else{

		// DISPLAY DATA AS KEY-VALUE

		// Get constants
		map<string, T> vals;
		jcalib->Get(NAMEPATH, vals);

		// Make one pass to find the maximum key width
		unsigned int max_key_width = 1;
		iter_T iter = vals.begin();
		for(; iter!=vals.end(); iter++){
			string key = iter->first;
			if(key.length()>max_key_width) max_key_width=key.length();
		}
		max_key_width++;

		for(iter=vals.begin(); iter!=vals.end(); iter++){
			string key = iter->first;
			cout<<iter->first<<" ";
			if(key.length()<max_key_width)
				for(unsigned int j=0; j<max_key_width-key.length(); j++)cout<<" ";
			cout<<iter->second;
			cout<<endl;
		}
	}
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(narg==1)Usage();

	for(int i=1;i<narg;i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
				case 'h':
					Usage();
					break;
				case 'R':
					if(strlen(argv[i])==2){
						if((i+1)<narg){
							RUN_NUMBER = atoi(argv[++i]);
						}else{
							cout<<"\"-R\" requires an argument!"<<endl;
						}
					}else{
						RUN_NUMBER = atoi(&argv[i][2]);
					}
					break;
				case 't':
					if(strlen(argv[i])==2){
						if((i+1)<narg){
							DATATYPE = string(argv[++i]);
						}else{
							cout<<"\"-t\" requires an argument!"<<endl;
						}
					}else{
						DATATYPE = string(&argv[i][2]);
					}
					break;
				case 'T':
					DISPLAY_AS_TABLE = true;
					break;
				case 'L':
					LIST_NAMEPATHS = true;
					break;
			}
		}else{
			NAMEPATH = argv[i];
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

	// Get list of available calibration generators
	vector<string> descriptions;
	if(japp){
		vector<JCalibrationGenerator*> generators = japp->GetCalibrationGenerators();
		for(unsigned int i=0; i<generators.size(); i++){
			descriptions.push_back(generators[i]->Description());
		}
	}

	// Print Usage statement
	cout<<"Usage:"<<endl;
	cout<<"       jcalibread [options] namepath"<<endl;
	cout<<endl;
	cout<<"Print calibration constants available via JANA"<<endl;
	cout<<"to the screen."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h        Print this message"<<endl;
	cout<<"   -R run    Set run number to \"run\""<<endl;
	cout<<"   -t type   Set data type (float, int, ...)"<<endl;
	cout<<"   -T        Display data as table"<<endl;
	cout<<"   -L        List available namepaths"<<endl;
	cout<<endl;
	cout<<endl;
	cout<<" Built-in available calibration sources:"<<endl;
	cout<<"  - JANA flat files"<<endl;
	for(unsigned int i=0; i<descriptions.size(); i++){
		cout<<"  - "<<descriptions[i]<<endl;
	}

	exit(0);
}


