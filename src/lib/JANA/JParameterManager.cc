//
//    File: JParameterManager.cc
// Created: Thu Oct 12 08:16:11 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include "JParameterManager.h"
#include "JLog.h"

using namespace std;

JParameterManager *gPARMS = nullptr;

//---------------------------------
// JParameterManager    (Constructor)
//---------------------------------
JParameterManager::JParameterManager()
{
	gPARMS = this;
}

//---------------------------------
// ~JParameterManager    (Destructor)
//---------------------------------
JParameterManager::~JParameterManager()
{
	for( auto p : _jparameters ) delete p.second;
	_jparameters.clear();
}

//---------------------------------
// Exists
//---------------------------------
bool JParameterManager::Exists(string name)
{
	return _jparameters.count(name) != 0;
}

//---------------------------------
// FindParameter
//---------------------------------
JParameter* JParameterManager::FindParameter(std::string name)
{	
	if( ! Exists(name) ) return nullptr;

	return _jparameters[name];
}		

//---------------------------------
// PrintParameters
//---------------------------------
void JParameterManager::PrintParameters(bool all)
{
	/// Print configuration parameters to stdout.
	/// If "all" is false (default) then only parameters
	/// whose values are different than their default are
	/// printed.
	/// If "all" is true then all parameters are 
	/// printed.

	// Find maximum key length
	uint32_t max_key_len = 4;
	vector<string> keys;
	for(auto &p : _jparameters){
		string key = p.first;
		auto j = p.second;
		if( (!all) && j->IsDefault() ) continue;
		keys.push_back( key );
		if( key.length()>max_key_len ) max_key_len = key.length();
	}
	
	// If all params are set to default values, then print a one line
	// summary
	if(keys.empty()){
		JLog() << "All configuration parameters set to default values." << JLogEnd();
		return;
	}

	// Print title/header
	string title("Config. Parameters");
	uint32_t half_title_len = 1+title.length()/2;
	if( max_key_len < half_title_len ) max_key_len = half_title_len;
	JLog() << "\n" << JLogEnd();
	JLog() << string(max_key_len+4-half_title_len, ' ') << title << "\n" << JLogEnd();
	JLog() << "  " << string(2*max_key_len + 3, '=') << "\n" << JLogEnd();
	JLog() << string(max_key_len/2, ' ') << "name" << string(max_key_len, ' ') << "value" << "\n" << JLogEnd();
	JLog() << "  " << string(max_key_len, '-') << "   " << string(max_key_len, '-') << "\n" << JLogEnd();

	// Print all parameters
	for(string &key : keys){
		string val = _jparameters[key]->GetValue<string>();
		JLog() << string(max_key_len+2-key.length(),' ') << key << " = " << val << "\n" << JLogEnd();
	}
	JLog() << "\n" << JLogEnd();
}


