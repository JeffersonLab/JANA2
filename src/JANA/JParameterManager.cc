// $Id: JParameterManager.cc 1229 2005-08-23 12:00:38Z davidl $
//
//    File: JParameterManager.cc
// Created: Tue Aug 16 14:30:24 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <ctype.h>
#include <pthread.h>
#include <fstream>
#include <iostream>
using namespace std;

#include "JParameterManager.h"
#include "JApplication.h"
using namespace jana;

#ifndef ansi_escape
#define ansi_escape			((char)0x1b)
#define ansi_bold 			ansi_escape<<"[1m"
#define ansi_italic 			ansi_escape<<"[3m"
#define ansi_underline 		ansi_escape<<"[4m"
#define ansi_blink 			ansi_escape<<"[5m"
#define ansi_rapid_blink	ansi_escape<<"[6m"
#define ansi_reverse			ansi_escape<<"[7m"
#define ansi_black			ansi_escape<<"[30m"
#define ansi_red				ansi_escape<<"[31m"
#define ansi_green			ansi_escape<<"[32m"
#define ansi_blue				ansi_escape<<"[34m"
#define ansi_normal			ansi_escape<<"[0m"
#define ansi_up(A)			ansi_escape<<"["<<(A)<<"A"
#define ansi_down(A)			ansi_escape<<"["<<(A)<<"B"
#define ansi_forward(A)		ansi_escape<<"["<<(A)<<"C"
#define ansi_back(A)			ansi_escape<<"["<<(A)<<"D"
#endif // ansi_escape


JParameterManager *gPARMS=NULL; // global pointer set to last instantiation
extern jana::JApplication *japp;

class JParameterAlphaSort{
	public:
		bool operator()(JParameter* const &p1, JParameter* const &p2) const {
			string s1(p1->GetKey());
			string s2(p2->GetKey());
			// I tried to get STL transform to work here, but couldn't ??
			for(unsigned int i=0;i<s1.size();i++)s1[i] = tolower(s1[i]);
			for(unsigned int i=0;i<s2.size();i++)s2[i] = tolower(s2[i]);
			//transform(s1.begin(), s1.end(), s1.begin(), tolower);
			//transform(s2.begin(), s2.end(), s2.begin(), tolower);
			return s1 < s2;
		}
};


//---------------------------------
// JParameterManager    (Constructor)
//---------------------------------
JParameterManager::JParameterManager()
{
	pthread_mutex_init(&parameter_mutex, NULL);
	printParametersCalled = false;
	string empty("");
	SetDefaultParameter("print", empty);
	verbose = false;
	
	gPARMS = this;
}

//---------------------------------
// ~JParameterManager    (Destructor)
//---------------------------------
JParameterManager::~JParameterManager()
{
	for(unsigned int i=0; i<parameters.size(); i++)delete parameters[i];
	parameters.clear();

	if(gPARMS==this)gPARMS=NULL;
}

//---------------------------------
// GetParameters
//---------------------------------
void JParameterManager::GetParameters(map<string,string> &parms, string filter)
{
	/// Copy a list of all parameters into the supplied map, replacing
	/// its current contents. If a filter string is supplied, it is
	/// used to filter out all parameters that are do not start with
	/// the filter string. In addition, the filter string is removed
	/// from the keys.
	
	parms.clear();
	pthread_mutex_lock(&parameter_mutex);
	for(unsigned int i=0; i<parameters.size(); i++){
		string key = parameters[i]->GetKey();
		string value = parameters[i]->GetValue();
		if(filter.size()>0){
			if(key.substr(0,filter.size())!=filter)continue;
			key.erase(0, filter.size());
		}
		parms[key] = value;
	}
	pthread_mutex_unlock(&parameter_mutex);
}

//---------------------------------
// ReadConfigFile
//---------------------------------
void JParameterManager::ReadConfigFile(string fname)
{
	/// Read in the configuration file with name specified by "fname".
	/// The file should have the form:
	///
	/// <pre>
	/// key1 value1
	/// key2 value2
	/// ...
	/// </pre>
	/// 
	/// Where there is a space between the key and the value (thus, the "key"
	/// can contain no spaces). The value is taken as the rest of the line
	/// up to, but not including the newline itself.
	///
	/// A key may be specified with no value and the value will be set to "1".
	///
	/// A "#" charater will discard the remaining characters in a line up to
	/// the next newline. Therefore, lines starting with "#" are ignored
	/// completely.
	///
	/// Lines with no characters (except for the newline) are ignored.

	// Try and open file
	ifstream ifs(fname.c_str());
	if(!ifs.is_open()){
		jerr<<"Unable to open configuration file \""<<fname<<"\" !"<<endl;
		exit(-1);
		return;
	}
	jout<<"Reading configuration from \""<<fname<<"\" ..."<<endl;
	
	// Loop over lines
	char line[1024];
	while(!ifs.eof()){
		// Read in next line ignoring comments 
		ifs.getline(line, 1024);
		if(strlen(line)==0)continue;
		if(line[0] == '#')continue;
		string str(line);

		// Check for comment character and erase comment if found
		size_t comment_pos = str.find('#');
		if(comment_pos!=string::npos){
		
			// Parameter descriptions are automatically added to configuration dumps
			// by adding a space, then the '#'. For string parameters, this extra
			// space shouldn't be there so check for it and delete it as well if found.
			if(comment_pos>0 && str[comment_pos-1]==' ')comment_pos--;

			str.erase(comment_pos);
		}

		// Break line into tokens
		vector<string> tokens;
		string buf; // Have a buffer string
		stringstream ss(str); // Insert the string into a stream
		while (ss >> buf)tokens.push_back(buf);
		if(tokens.size()<1)continue; // ignore empty lines

		// Use first token as key
		string key = tokens[0];
		
		// Concatenate remaining tokens into val string
		string val="";
		for(unsigned int i=1; i<tokens.size(); i++){
			if(i!=1)val += " ";
			val += tokens[i];
		}
		if(val=="")val="1";

		// Set Configuration Parameter
		SetParameter(key, val);
	}
	
	// close file
	ifs.close();	
}

//---------------------------------
// WriteConfigFile
//---------------------------------
void JParameterManager::WriteConfigFile(string fname)
{
	/// Write all of the configuration parameters out to an ASCII file in
	/// a format compatible with reading in via ReadConfigFile().

	// Try and open file
	ofstream ofs(fname.c_str());
	if(!ofs.is_open()){
		jerr<<"Unable to open configuration file \""<<fname<<"\" for writing!"<<endl;
		return;
	}
	jout<<"Writing configuration parameters to \""<<fname<<"\" ..."<<endl;
	
	// Write header
	time_t t = time(NULL);
	string timestr(ctime(&t));
	vector<string> args;
	if(japp) args = japp->GetArgs();
	ofs<<"#"<<endl;
	ofs<<"# JANA Configuration parameters (auto-generated)"<<endl;
	ofs<<"#"<<endl;
	ofs<<"# created: "<<timestr;
	ofs<<"# command:";
	for(unsigned int i=0; i<args.size(); i++)ofs<<" "<<args[i];
	ofs<<endl;
	ofs<<"#"<<endl;
	ofs<<endl;
	
	// Lock mutex to prevent manipulation of parameters while we're writing
	pthread_mutex_lock(&parameter_mutex);
	
	// Sort parameters alphabetically
	std::sort(parameters.begin(), parameters.end(), JParameterAlphaSort());

	// First, find the longest key and value
	unsigned int max_key_len = 0;
	unsigned int max_val_len = 0;
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];		
		if(p->GetKey().length()>max_key_len) max_key_len = p->GetKey().length(); 
		if(p->GetValue().length()>max_val_len) max_val_len = p->GetValue().length(); 
	}
	
	// Loop over parameters a second time and print them out
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];
		string key = p->GetKey();
		string val = p->GetValue();
		string line = key;
		if(val.length()>0) line += string(max_key_len-key.length(),' ') + " " + val + string(max_val_len-val.length(),' ');

		// If there's a description, add it as a comment
		if(p->GetDescription() != "")line += string(" # ")+p->GetDescription();

		// Print the parameter
		ofs<<line.c_str()<<endl;
	}
	
	// Release mutex
	pthread_mutex_unlock(&parameter_mutex);
	
	// close file
	ofs.close();	
}

//---------------------------------
// PrintParameters
//---------------------------------
void JParameterManager::PrintParameters(void)
{
	// Every JEventLoop(thread) will try calling us. The first one should
	// print the parameters while the rest should not.

	// Block other threads from checking printParametersCalled while we do
	pthread_mutex_lock(&parameter_mutex);
	
	if(printParametersCalled){
		pthread_mutex_unlock(&parameter_mutex);
		return;
	}
	printParametersCalled = true;
	
	// release the parameters mutex (we need to do this before the GetParameter() call below)
	pthread_mutex_unlock(&parameter_mutex);

	if(parameters.size() == 0){
		jout<<" - No configuration parameters defined -"<<std::endl;
		return;
	}
	
	// Some parameters used to control what we do here
	// (does this seem incestuous?)
	string filter("");
	GetParameter("print", filter);
	bool printAll = filter == "all";
	
	// Lock mutex 
	pthread_mutex_lock(&parameter_mutex);

	// Sort parameters alphabetically
	std::sort(parameters.begin(), parameters.end(), JParameterAlphaSort());
	
	jout<<std::endl;
	jout<<" --- Configuration Parameters --"<<std::endl;
	
	// First, find the longest key and value and set the "printme" flags
	unsigned int max_key_len = 0;
	unsigned int max_val_len = 0;
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];
		string key = p->GetKey();
		p->printme = false;
		
		if(printAll)p->printme = true;
		if(filter.length())
			if(filter == key.substr(0,filter.length()))p->printme = true;
		if(!p->isdefault)p->printme = true;

		if(!p->printme)continue;

		if(p->GetKey().length()>max_key_len && p->GetKey().length()<80) max_key_len = p->GetKey().length();
		if(p->GetValue().length()>max_val_len && p->GetValue().length()<80) max_val_len = p->GetValue().length();
	}
	
	if(max_key_len==0) max_key_len=28;
	if(max_val_len==0) max_val_len=32;

	// Special prefixes to *not* make loud warning messages about
	vector<string> no_warn;
	no_warn.push_back("DEFTAG:");
	no_warn.push_back("JANA:");
	no_warn.push_back("EVENTS_TO_SKIP");
	no_warn.push_back("EVENTS_TO_KEEP");
	no_warn.push_back("SKIP_TO_EVENT");
	no_warn.push_back("EVENT_SOURCE_TYPE");
	no_warn.push_back("MAX_EVENTS_IN_BUFFER");
	no_warn.push_back("THREAD_TIMEOUT");
	no_warn.push_back("THREAD_TIMEOUT_FIRST_EVENT");
	no_warn.push_back("THREAD_STALL_WARN_TIMEOUT");
	no_warn.push_back("RECORD_CALL_STACK");
	no_warn.push_back("PRINT_PLUGIN_PATHS");
	no_warn.push_back("PLUGINS");
	no_warn.push_back("AUTOACTIVATE");
	no_warn.push_back("NTHREADS");
	no_warn.push_back("JANADOT:GROUP:");
	no_warn.push_back("JANADOT:FOCUS");
	no_warn.push_back("JANADOT:SUPPRESS_UNUSED_FACTORIES");
	
	// Loop over parameters a second time and print them out
	int Nprinted = 0;
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];
		if(!p->printme)continue;
		Nprinted++;
		string key = p->GetKey();
		string val = p->GetValue();
		string line = " " + key;
		if(key.length() < max_key_len) line+=string(max_key_len-key.length(),' ');
		line += " = " + val;
		if(val.length() < max_val_len) line+=string(max_val_len-val.length(),' ');

		// Warn if value has been set without specifying a default ...
		bool warn = (!p->hasdefault);
		
		// ... unless it is a "special" parameter used internally by JANA
		for(unsigned int j=0; j<no_warn.size(); j++){
			if(key.substr(0,no_warn[j].size())==no_warn[j])warn=false;
		}

		// Print the parameter
		if(!p->isdefault)jout<<ansi_bold;
		if(warn)jout<<ansi_red<<ansi_bold<<ansi_blink;
		jout<<line.c_str();
		if(warn)jout<<" <-- NO DEFAULT! (TYPO?)"<<ansi_normal;
		if(!p->isdefault)jout<<ansi_normal;
		jout<<std::endl;
	}
	
	if(!Nprinted)jout<<"        < all defaults >"<<std::endl;
	
	jout<<" -------------------------------"<<std::endl;

	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);
}

//---------------------------------
// Dump
//---------------------------------
void JParameterManager::Dump(void)
{
	// Just in case a new parameter is written while we're dumping
	pthread_mutex_lock(&parameter_mutex);

	// Call Dump() for all parameters
	for(unsigned int i=0; i<parameters.size(); i++){
		parameters[i]->Dump();
	}
	
	pthread_mutex_unlock(&parameter_mutex);

}

//---------------------------------
// DumpSuccinct
//---------------------------------
void JParameterManager::DumpSuccinct(bool print_descriptions)
{
	/// Draw one line to stdout for each parameter. Optionally print
	/// descriptions as well. If descriptions are printed, then
	/// the width of the terminal screen is used to break up the
	/// description into multiple lines to better fit it to the
	/// screen.

	// Get size of terminal
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
//	int Nrows = w.ws_row;
	int Ncols = w.ws_col;

	// Just in case a new parameter is written while we're dumping
	pthread_mutex_lock(&parameter_mutex);

	// Get maximum key width for all parameters
	uint32_t max_key_width=0;
	uint32_t max_val_width=0;
	for(unsigned int i=0; i<parameters.size(); i++){

		JParameter *p = parameters[i];
		const string &key = p->GetKey();
		const string &val = p->GetValue();

		if(key.length() > max_key_width) max_key_width = key.length();
		if(val.length() > max_val_width) max_val_width = val.length();
	}
	
	// Limit size so one really long key or value doesn't make ugly spacing
	// for everyone else!
	if(max_key_width > 48) max_key_width = 48;
	if(max_val_width > 32) max_val_width = 32;

	cout << endl;

	// print all parameters
	for(unsigned int i=0; i<parameters.size(); i++){
	
		JParameter *p = parameters[i];
		const string &key = p->GetKey();
		const string &val = p->GetValue();
		const string &description = p->GetDescription();

		int Nspaces = max_key_width - key.length();
		if(Nspaces<0) Nspaces=0;

		// print key = val
		cout << string(Nspaces, ' ') << key << " = " << val;

		// optionally print description
		if(print_descriptions) {
			Nspaces = max_val_width - val.length(); 
			if(Nspaces>0) cout << string(Nspaces, ' ');
			
			// Break description up into multiple lines
			int Nspaces_descr = max_key_width + max_val_width + 3; // +3 is for " = "
			int max_width = Ncols - Nspaces_descr - 3;
			if(max_width<12) max_width = 12;
			
			size_t pos_end = 0;
			while(true){
				size_t pos_start = pos_end;
				size_t pos = pos_start;
				while(true){
					pos = description.find(" ", pos+1);
					if(pos == string::npos) pos = description.length();
					if( (pos-pos_start) > (size_t)max_width) break;
					pos_end = pos;
					if(pos_end==0) break; // empty string
					if(pos_end==description.length()) break;
					if(pos_end==string::npos) break;
				}
				if(pos_end == pos_start) pos_end = string::npos;
				if(pos_start!=0) cout << string(Nspaces_descr, ' ');
				cout << " # " << description.substr(pos_start, pos_end - pos_start) << endl;
				if(pos_end == string::npos) break;
				if(pos_end >= description.length()) break;
			}
		}
	}
	cout << endl;
	
	pthread_mutex_unlock(&parameter_mutex);

}

