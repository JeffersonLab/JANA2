// $Id: JParameterManager.cc 1229 2005-08-23 12:00:38Z davidl $
//
//    File: JParameterManager.cc
// Created: Tue Aug 16 14:30:24 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <ctype.h>
#include <pthread.h>

#include "JParameterManager.h"

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
		cout<<" - No configuration parameters defined -"<<endl;
		return;
	}
	
	// Some parameters used to control what we do here
	// (does this seem incestuous?)
	string filter("");
	GetParameter("print", filter);
	bool printAll = filter == "all";
	
	// Sort parameters alphabetically
	sort(parameters.begin(), parameters.end(), JParameterAlphaSort());
	
	cout<<endl;
	cout<<" --- Configuration Parameters --"<<endl;
	
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
	
	// Loop over parameters a second time and print them out
	int Nprinted = 0;
	for(unsigned int i=0; i<parameters.size(); i++){
		JParameter *p = parameters[i];
		if(!p->printme)continue;
		Nprinted++;
		string key = p->GetKey();
		string val = p->GetValue();
		string line = " " + key + string(max_key_len-key.length(),' ') + " = " + val + string(max_val_len-val.length(),' ');

		if(!p->isdefault)cout<<ansi_bold;
		if(!p->hasdefault)cout<<ansi_red<<ansi_bold<<ansi_blink;
		cout<<line.c_str();
		if(!p->hasdefault)cout<<" <-- NO DEFAULT! (TYPO?)"<<ansi_normal;
		if(!p->isdefault)cout<<ansi_normal;
		cout<<endl;
	}
	
	if(!Nprinted)cout<<"        < all defaults >"<<endl;
	
	cout<<" -------------------------------"<<endl;
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
