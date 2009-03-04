// $Id: JParameterManager.cc 1229 2005-08-23 12:00:38Z davidl $
//
//    File: JParameterManager.cc
// Created: Tue Aug 16 14:30:24 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

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
	
	gPARMS = this;
}

//---------------------------------
// ~JParameterManager    (Destructor)
//---------------------------------
JParameterManager::~JParameterManager()
{
	for(unsigned int i=0; i<parameters.size(); i++)delete parameters[i];
	parameters.clear();

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
	for(unsigned int i=0; i<parameters.size(); i++){
		string key = parameters[i]->GetKey();
		string value = parameters[i]->GetValue();
		if(filter.size()>0){
			if(key.substr(0,filter.size())!=filter)continue;
			key.erase(0, filter.size());
		}
		parms[key] = value;
	}
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
		cerr<<"Unable to open configuration file \""<<fname<<"\" !"<<endl;
		return;
	}
	cout<<"Reading configuration from \""<<fname<<"\" ..."<<endl;
	
	// Loop over lines
	char line[1024];
	while(!ifs.eof()){
		// Read in next line ignoring comments 
		ifs.getline(line, 1024);
		if(strlen(line)==0)continue;
		if(line[0] == '#')continue;
		string str(line);

		// Check for comment character and erase comment if found
		if(str.find('#')!=str.npos)str.erase(str.find('#'));

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
		cerr<<"Unable to open configuration file \""<<fname<<"\" for writing!"<<endl;
		return;
	}
	cout<<"Writing configuration parameters to \""<<fname<<"\" ..."<<endl;
	
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

		// Print the parameter
		ofs<<line.c_str()<<endl;
	}
	
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
	
	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);


	if(parameters.size() == 0){
		std::cout<<" - No configuration parameters defined -"<<std::endl;
		return;
	}
	
	// Some parameters used to control what we do here
	// (does this seem incestuous?)
	string filter("");
	GetParameter("print", filter);
	bool printAll = filter == "all";
	
	// Sort parameters alphabetically
	std::sort(parameters.begin(), parameters.end(), JParameterAlphaSort());
	
	std::cout<<std::endl;
	std::cout<<" --- Configuration Parameters --"<<std::endl;
	
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

		if(p->GetKey().length()>max_key_len) max_key_len = p->GetKey().length(); 
		if(p->GetValue().length()>max_val_len) max_val_len = p->GetValue().length(); 
	}

	// Special prefixes to *not* make loud warning messages about
	vector<string> no_warn;
	no_warn.push_back("DEFTAG:");
	no_warn.push_back("JANA:");
	no_warn.push_back("EVENTS_TO_SKIP");
	no_warn.push_back("EVENTS_TO_KEEP");
	no_warn.push_back("THREAD_TIMEOUT");
	no_warn.push_back("RECORD_CALL_STACK");
	no_warn.push_back("PRINT_PLUGIN_PATHS");
	
	// Loop over parameters a second time and print them out
	int Nprinted = 0;
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];
		if(!p->printme)continue;
		Nprinted++;
		string key = p->GetKey();
		string val = p->GetValue();
		string line = " " + key + string(max_key_len-key.length(),' ') + " = " + val + string(max_val_len-val.length(),' ');

		// Warn if value has been set without specifying a default ...
		bool warn = (!p->hasdefault);
		
		// ... unless it is a "special" parameter used internally by JANA
		for(unsigned int j=0; j<no_warn.size(); j++){
			if(key.substr(0,no_warn[j].size())==no_warn[j])warn=false;
		}

		// Print the parameter
		if(!p->isdefault)std::cout<<ansi_bold;
		if(warn)std::cout<<ansi_red<<ansi_bold<<ansi_blink;
		std::cout<<line.c_str();
		if(warn)std::cout<<" <-- NO DEFAULT! (TYPO?)"<<ansi_normal;
		if(!p->isdefault)std::cout<<ansi_normal;
		std::cout<<std::endl;
	}
	
	if(!Nprinted)std::cout<<"        < all defaults >"<<std::endl;
	
	std::cout<<" -------------------------------"<<std::endl;
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
