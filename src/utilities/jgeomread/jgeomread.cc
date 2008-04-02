// Author: David Lawrence  June 25, 2004
//
//
// hd_ana.cc
//

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
#include <JANA/JGeometry.h>

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

unsigned int RUN_NUMBER = 100;
string XPATH="";
bool LIST_XPATHS=false;
bool QUIET = false;
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
	
	// Get and display values for a specific namepath if one was specified
	if(XPATH!=""){
		
		// Determine whether an attribute was specified at the end of the xpath
		bool attribute_specified = false;
		string attribute="";
		string::size_type pos_slash = XPATH.find_last_of("/", XPATH.size());
		if(pos_slash!=string::npos && (pos_slash+1)!=XPATH.size()){
			if(XPATH[pos_slash+1] == '@'){
				attribute_specified = true;
				attribute = XPATH.substr(pos_slash+2, XPATH.size()-(pos_slash+2));
			}
		}

		// If an attribute name was specified, get and print the value for
		// that specific attribute. Otherwise, get all attributes of the
		// specified xpath and print them.
		map<string, string> vals;
		if(attribute_specified){
			string sval;
			if(!jgeom->Get(XPATH, sval)){
				cerr<<"Couldn't find the specified node."<<endl;
				return -1;
			}else{
				vals[attribute] = sval;
			}
		}else{
			// Get constants
			if(!jgeom->Get(XPATH, vals)){
				cerr<<"No node found matching the given xpath."<<endl;
				return -1;
			}
		}

		// Display constants
		if(!QUIET)cout<<endl<<"Values for \""<<XPATH<<"\" for run "<<RUN_NUMBER<<endl;
		if(!QUIET)cout<<"--------------------"<<endl;
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
	
	delete app;

	return 0;
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
				case 'R':
					if(strlen(argv[i])==2){
						RUN_NUMBER = atoi(argv[++i]);
					}else{
						RUN_NUMBER = atoi(&argv[i][2]);
					}
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
	cout<<"   -t type   Set data type (float, int, ...)"<<endl;
	cout<<endl;
	cout<<" Example: (note this relies on the structure of the backend)"<<endl;
	cout<<endl;
	cout<<"    jgeomread '//hdds:element[@name=\"Antimony\"]/@a'"<<endl;
	cout<<endl;

	exit(0);
}


