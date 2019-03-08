//
//    File: JParameterManager.h
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
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include <JANA/JApplication.h>

#ifndef _JParameterManager_h_
#define _JParameterManager_h_

#include <string>
#include <algorithm>
#include <map>

#include <JANA/JParameter.h>
#include <JANA/JException.h>

class JParameterManager;

class JParameterManager{
	public:
		JParameterManager();
		virtual ~JParameterManager();
		
		bool Exists(std::string name);
		JParameter* FindParameter(std::string);
		void PrintParameters(bool all=false);
		std::size_t GetNumParameters(void){ return _jparameters.size(); }

		template<typename T>
		JParameter* GetParameter(std::string name, T &val);

		template<typename T>
		T GetParameterValue(std::string name);
		
		template<typename K, typename V>
		JParameter* SetDefaultParameter(K key, V& val, std::string description="");

		template<typename T>
		JParameter* SetParameter(std::string name, T val);

	protected:
	
		// When accessing the _jparameters map strings are always converted to
		// lower case. This effectively makes configuration parameters case-insensitve
		// while allowing users to use case and have that stored as the actual parameter
		// name
		string ToLC(string &name){std::string tmp(name); std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower); return tmp;}
		std::map<std::string, JParameter*> _jparameters;
	
	private:

};

//---------------------------------
// GetParameter
//---------------------------------
template<typename T>
JParameter* JParameterManager::GetParameter(std::string name, T &val)
{	
	if( ! Exists(name) ) return NULL;

	auto jpar = _jparameters[ ToLC(name) ];
	jpar->GetValue( val );
	return jpar;
}		

//---------------------------------
// GetParameterValue
//---------------------------------
template<typename T>
T JParameterManager::GetParameterValue(std::string name)
{	
	if( ! Exists(name) ) throw JException("Unknown parameter \"%s\"", name.c_str());

	return _jparameters[ ToLC(name) ]->GetValue<T>();
}		

//---------------------------------
// SetDefaultParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::SetDefaultParameter(K key, V &val, std::string description)
{
	/// Retrieve a configuration parameter, creating it if necessary.
	///
	/// Upon entry, the value in "val" should be set to the desired default value. It
	/// will be overwritten if a value for the parameter already exists because
	/// it was given by the user either on the command line or in a configuration
	/// file.
	///
	/// If the parameter does not already exist, it is created and its value set
	/// to that of "val".
	///
	/// Upon exit, "val" will always contain the value that should be used for event
	/// processing.
	///
	/// If a parameter with the given name already exists, it will be checked to see
	/// if the parameter already has a default value assigned (this is kept separate
	/// from the actual value of the parameter used and is maintained purely for
	/// bookkeeping purposes). If it does not have a default value, then the value
	/// of "val" upon entry is saved as the default. If it does have a default, then
	/// the value of the default is compared to the value of "val" upon entry. If the
	/// two do not match, then a warning message is printed to indicate to the user
	/// that two different default values are being set for this parameter.
	///
	/// Parameters specified on the command line using the "-Pkey=value" syntax will
	/// not have a default value at the time the parameter is created.
	///
	/// This should be called after the JApplication object has been initialized so
	/// that parameters can be created from any command line options the user may specify.

	auto p = FindParameter(key);
	if( p != nullptr ){
		
		if( p->GetHasDefault() ){
			auto s1 = p->template GetDefault<std::string>(); // yes, this crazy syntax is correct
			auto s2 = p->template GetValue<std::string>();   // yes, this crazy syntax is correct
			if( s1 != s2 ){
				throw JException("WARNING: Multiple calls to SetDefaultParameter with key=\"%s\": %s != %s", key, s1.c_str(), s2.c_str());
			}
		}else{
			p->SetDefault( val );
		}
		
	}else{
		p = SetParameter(key, val);
		p->SetDefault( val );
	}

	val = p->template GetValue<V>();

	return p;
	
#if 0
	string skey(key); // (handle const char* or string)
	stringstream ss;
	ss << std::setprecision(15) << val;
	string sval = ss.str();
	
	pthread_mutex_lock(&parameter_mutex);
	
	JParameter *p = GetParameterNoLock(key);
	if(p != NULL){
		// Parameter exists
		
		// Copy value into user's variable using stringstream for conversion
		p->GetValue(val);
		
		// Warn user if two different default values are set
		if(p->hasdefault && (sval != p->GetDefault()) ){
			jout<<" WARNING: Multiple calls to SetDefaultParameter with key=\""
			<<key<<"\" value= \""<<p->GetDefault()<<"\" and \""<<sval<<"\""<<std::endl;
			jout<<"        : (\""<<sval<<"\" will be used for the default.)"<<std::endl;
		}
		
		if(!p->hasdefault){
			// Parameters set from the command line will have the
			// wrong data type since SetParameter will have been called
			// with a string type for the value. If a default has not
			// been set already for this parameter, then we assume the
			// currently set data type is invalid and we replace it with
			// the type specified in this call.
			p->type = JParameter::DataType(val);
		}		
	}else{
		// Parameter doesn't exist. Create it.
		p = new JParameter(skey, sval);
		parameters.push_back(p);
		p->type = JParameter::DataType(val);
		
		// We want the value used by this thread to be exactly the same as the
		// the value for susequent threads. Since they will get a value that has
		// been converted to/from a string, we need to do this here as well.
		V save_val = val;
		p->GetValue(val);
		
		// Warn the user if the conversion ends up changing the value
		if(val != save_val){
			// Use dedicated stringstream objects to convert using high precision
			// to avoid changing the prescision setting of jerr
			stringstream ss_bef;
			stringstream ss_aft;
			ss_bef << std::setprecision(15) << save_val;
			ss_aft << std::setprecision(15) << val;
		
			jerr<<" WARNING! The value for "<<skey<<" is changed while storing and retrieving parameter default"<<std::endl;
			jerr<<"          before conversion:"<< ss_bef.str() << std::endl;
			jerr<<"          after  conversion:"<< ss_aft.str() << std::endl;
		}
	}
	
	// Set the default value and description for this parameter
	p->SetDefault(sval);
	p->SetDescription(description);
	
	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);

	return p;
#endif
}

//---------------------------------
// SetParameter
//---------------------------------
template<typename T>
JParameter* JParameterManager::SetParameter(std::string name, T val)
{	
	if( !Exists(name) ) {
		_jparameters[ ToLC(name) ] = new JParameter(name, val);
	}else{
		_jparameters[ ToLC(name) ]->SetValue( val );
	}

	return _jparameters[ ToLC(name) ];
}		

#endif // _JParameterManager_h_

