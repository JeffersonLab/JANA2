// Author: David Lawrence  June 25, 2004
//
//
// jgeomread.cc
//

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
#include <JANA/JGeometry.h>
using namespace jana;

void DisplayMultiple(JGeometry *jgeom, bool attribute_specified, string attribute);
void DisplaySingle(JGeometry *jgeom, bool attribute_specified, string attribute);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

unsigned int RUN_NUMBER = 100;
string XPATH="";
bool LIST_XPATHS = false;
bool FILTER_XPATHS = false;
bool QUIET = false;
bool DISPLAY_MULTIPLE = false;
bool PRINT_CHECKSUM = false;
bool PRINT_CHECKSUM_INPUT_FILES = false;
JGeometry::ATTR_LEVEL_t ATTRIBUTE_LEVEL=JGeometry::attr_level_last;

//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	// Parse the command line
	ParseCommandLineArguments(narg, argv);

	// Instantiate a JApplication object and use it to get a JGeometry object
	JApplication *app = new JApplication(narg, argv);
	gPARMS->SetDefaultParameter("PRINT_CHECKSUM_INPUT_FILES", PRINT_CHECKSUM_INPUT_FILES, "Have the JGeometry object print the XML file names used.");
	JGeometry *jgeom = app->GetJGeometry(RUN_NUMBER);
	
	// List all valid namepaths, if specified
	if(LIST_XPATHS){
		vector<string> xpaths;
		jgeom->GetXPaths(xpaths, ATTRIBUTE_LEVEL);
		cout<<endl;
		cout<<"List of all valid xpaths for url=\""<<jgeom->GetURL()<<"\" ("<<xpaths.size()<<" total)"<<endl;
		cout<<"----------------------------------------------------------------"<<endl;
		for(unsigned int i=0; i<xpaths.size(); i++)cout<< xpaths[i] <<endl;
		cout<<endl;
	}
	
	// List all xpaths that pass the given xpath as a filter
	if(FILTER_XPATHS && XPATH!=""){
		vector<string> xpaths;
		jgeom->GetXPaths(xpaths, ATTRIBUTE_LEVEL, XPATH);
		cout<<endl;
		cout<<"List of all valid (filtered) xpaths for url=\""<<jgeom->GetURL()<<"\" ("<<xpaths.size()<<" total)"<<endl;
		cout<<"----------------------------------------------------------------"<<endl;
		for(unsigned int i=0; i<xpaths.size(); i++)cout<< xpaths[i] <<endl;
		cout<<endl;
		return 0;
	}
	
	// Print the MD5 Checksum
	if(PRINT_CHECKSUM){
		cout<<"checksum: "<<jgeom->GetChecksum()<<endl;
	}
	
	// Get and display values for a specific namepath if one was specified
	if(XPATH!=""){
		
		// Determine whether an attribute was specified by searching the xpath
		// for an '@' for which there is no '=' before the next '/' or the end
		// of the xpath.
		bool attribute_specified = false;
		string attribute="";
		string::size_type pos = 0;
		while((pos=XPATH.find("@", pos)) != string::npos){
			pos++;
			string::size_type pos_equals = XPATH.find("=", pos);
			string::size_type pos_slash  = XPATH.find("/", pos);
			if(pos_equals < pos_slash) continue;
			if(pos_slash == string::npos) pos_slash = XPATH.size();
			attribute = XPATH.substr(pos, pos_slash-pos);
			attribute_specified = true;
			break;
		}
		
		// Display results
		if(!QUIET)cout<<endl<<"Values for \""<<XPATH<<"\" for run "<<RUN_NUMBER<<endl;
		if(!QUIET)cout<<"--------------------"<<endl;
		if(DISPLAY_MULTIPLE){
			DisplayMultiple(jgeom, attribute_specified, attribute);
		}else{
			DisplaySingle(jgeom, attribute_specified, attribute);
		}

	}
	
	int exit_code = app->GetExitCode();

	delete app;

	return exit_code;
}

//-----------
// DisplaySingle
//-----------
void DisplaySingle(JGeometry *jgeom, bool attribute_specified, string attribute)
{
	// If an attribute name was specified, get and print the value for
	// that specific attribute. Otherwise, get all attributes of the
	// specified xpath and print them.
	map<string, string> vals;
	if(attribute_specified){
		string sval;
		if(!jgeom->Get(XPATH, sval)){
			cerr<<"Couldn't find the specified node."<<endl;
			exit(-1);
		}else{
			vals[attribute] = sval;
		}
	}else{
		// Get constants
		if(!jgeom->Get(XPATH, vals)){
			cerr<<"No node found matching the given xpath."<<endl;
			exit(-1);
		}
	}

	// Display constants
	map<string, string>::iterator iter;
	
	// Make one pass to find the maximum key width
	unsigned int max_key_width = 1;
	for(iter=vals.begin(); iter!=vals.end(); iter++){
		string key = iter->first;
		if(key.length()>max_key_width) max_key_width=key.length();
	}
	max_key_width++;

	for(iter=vals.begin(); iter!=vals.end(); iter++){
		string key = iter->first;
		string val = iter->second;
		if(!QUIET){
			cout<<iter->first<<" ";
			if(key.length()<max_key_width)
				for(unsigned int j=0; j<max_key_width-key.length(); j++)cout<<" ";
		}
		cout<<iter->second;
		cout<<endl;
	}
}

//-----------
// DisplayMultiple
//-----------
void DisplayMultiple(JGeometry *jgeom, bool attribute_specified, string attribute)
{
	// If an attribute name was specified, get and print the value for
	// that specific attribute. Otherwise, get all attributes of the
	// specified xpath and print them.
	vector<map<string, string> > vals;
	if(attribute_specified){
		vector<string> svals;
		if(!jgeom->GetMultiple(XPATH, svals)){
			cerr<<"Couldn't find the specified node."<<endl;
			exit(-1);
		}else{
			for(unsigned int i=0; i<svals.size(); i++){
				map<string,string> tmp;
				tmp[attribute] = svals[i];
				vals.push_back(tmp);
			}
		}
	}else{
		// No attribute was specified
		if(!jgeom->GetMultiple(XPATH, vals)){
			cerr<<"No node found matching the given xpath."<<endl;
			exit(-1);
		}
	}

	// Make one pass to find the maximum key width
	unsigned int max_key_width = 1;
	map<string, string>::iterator iter;
	for(unsigned int i=0; i<vals.size(); i++){
		for(iter=vals[i].begin(); iter!=vals[i].end(); iter++){
			string key = iter->first;
			if(key.length()>max_key_width) max_key_width=key.length();
		}
	}
	max_key_width++;

	// Print values
	for(unsigned int i=0; i<vals.size(); i++){
		if(!QUIET)cout<<"----- "<<i<<endl;
		for(iter=vals[i].begin(); iter!=vals[i].end(); iter++){
			string key = iter->first;
			string val = iter->second;
			if(!QUIET){
				cout<<iter->first<<" ";
				if(key.length()<max_key_width)
					for(unsigned int j=0; j<max_key_width-key.length(); j++)cout<<" ";
			}
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
				case 'q':
					QUIET = true;
					break;
				case 'L':
					LIST_XPATHS = true;
					break;
				case 'a':
					ATTRIBUTE_LEVEL = JGeometry::attr_level_all;
					break;
				case 'b':
					ATTRIBUTE_LEVEL = JGeometry::attr_level_last;
					break;
				case 'c':
					ATTRIBUTE_LEVEL = JGeometry::attr_level_none;
					break;
				case 'f':
					FILTER_XPATHS = true;
					break;
				case 'm':
					DISPLAY_MULTIPLE = true;
					break;
				case 'R':
					if(strlen(argv[i])==2){
						RUN_NUMBER = atoi(argv[++i]);
					}else{
						RUN_NUMBER = atoi(&argv[i][2]);
					}
					break;
				case 'S':
					PRINT_CHECKSUM_INPUT_FILES = true;
					// don't break here!
				case 's':
					PRINT_CHECKSUM = true;
					break;
			}
		}else{
			XPATH = argv[i];
		}
	}
}

//-----------
// Usage
//-----------
void Usage(void)
{
	cout<<"Usage:"<<endl;
	cout<<"       jgeomread [options] xpath"<<endl;
	cout<<endl;
	cout<<"Print the contents of a JANA geometry source (e.g. a file)"<<endl;
	cout<<"to the screen."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h        Print this message"<<endl;
	cout<<"   -q        Quiet mode. Print only the values separated by newlines."<<endl;
	cout<<"   -L        List all xpaths"<<endl;
	cout<<"   -a        Include attributes for all nodes when listing xpaths"<<endl;
	cout<<"   -b        Include attributes for only the last node when listing xpaths(default)"<<endl;
	cout<<"   -c        Don't include any attributes listing xpaths"<<endl;
	cout<<"   -f        List all xpaths using the specified xpath as a filter"<<endl;
	cout<<"   -m        Display all nodes satisfying the given xpath"<<endl;
	cout<<"   -t type   Set data type (float, int, ...)"<<endl;
	cout<<"   -s        Print the MD5 checksum of the XML used"<<endl;
	cout<<"   -S        Same as -s except filenames of XML files are also printed"<<endl;
	cout<<endl;
	cout<<" A note on the use of -f vs. -m: If -m is specified, then"<<endl;
	cout<<"the values are obtained using one of the GetMultiple() methods"<<endl;
	cout<<"of JCalibration, otherwise on of the Get() methods is used."<<endl;
	cout<<"The attribute values for all matching nodes are then printed."<<endl;
	cout<<" "<<endl;
	cout<<"The -f argument will get and display a list of xpaths themselves "<<endl;
	cout<<"but filtering them based on the given filter."<<endl;
	cout<<" "<<endl;
	cout<<endl;
	cout<<" Example: (note this relies on the structure of the backend)"<<endl;
	cout<<endl;
	cout<<"    jgeomread '//hdds:element[@name=\"Antimony\"]/@a'"<<endl;
	cout<<endl;

	exit(0);
}


