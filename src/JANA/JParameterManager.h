// $Id: JParameterManager.h 1230 2005-08-23 13:17:11Z davidl $
//
//    File: JParameterManager.h
// Created: Tue Aug 16 14:30:24 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JParameterManager_
#define _JParameterManager_

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
using std::vector;
using std::string;
using std::stringstream;
using std::map;

#include <pthread.h>

#include <JANA/JParameter.h>
#include <JANA/JStreamLog.h>

#include "jerror.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#ifdef __CINT__
#include "cint.h"
#endif


// Place everything in JANA namespace
namespace jana{


/// The JParameterManager class is used by the framework to manage
/// configuration parameters in the form of JParameter objects. The
/// factory author will implement a configuration parameter in one
/// of the factory's callback routines (usually init(...)) via the
/// SetDefaultParameter(...) method of JParameterManager. There is
/// only one JParameterManager object per application. It is
/// automatically instantiated by the JApplication object. A global
/// pointer to the JParameterManager object called "jparms" is
/// available to make it more easily accessible.
///
/// 

class JParameterManager{
	public:
		JParameterManager();
		virtual ~JParameterManager();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JParameterManager";}
	
		template<typename K, typename V> JParameter* SetDefaultParameter(K key, V& val); ///< Set a configuration parameter's default value
		template<typename K, typename V> JParameter* SetParameter(K key, V val); ///< Force-set a value for a configuration parameter
		template<typename K> JParameter* GetParameterNoLock(K key); ///< Get the value of a configuration parameter without locking the mutex
		template<typename K> JParameter* GetParameter(K key); ///< Get JParameter object for a configuration parameter
		template<typename K, typename V> JParameter* GetParameter(K key, V &val); ///< Get value of a parameter and its JParameter Object
		template<typename K> JParameter* GetParameter(K key, string &val); ///< Get value of a parameter and its JParameter Object
		void GetParameters(map<string,string> &parms, string filter="");
		void ReadConfigFile(string fname);
		void WriteConfigFile(string fname);
		void PrintParameters(void); ///< Print a list of the configuration parameters
		void Dump(void); ///< Invoke the Dump() method of all JParameter objects
		
	private:
		vector<JParameter*> parameters;
		pthread_mutex_t parameter_mutex;
		bool printParametersCalled;

};


//---------------------------------
// SetDefaultParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::SetDefaultParameter(K key, V &val)
{
	stringstream ss;
	ss<<val;
	string sval = ss.str();
	
	JParameter *p = GetParameter(key,val);
	if(!p){
		p = SetParameter(key, val);
	}else{
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
	}

	// The call to SetDefault() below can cause a reallocation
	// of a string's buffer. If two threads do this simulataneously,
	// you can get a "double free" followed by a seg. fault.
	pthread_mutex_lock(&parameter_mutex);

	// Set the default for this parameter
	p->SetDefault(sval);

	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);

	return p;
}

//---------------------------------
// SetParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::SetParameter(K key, V val)
{
	stringstream ss; // use a stringstream to convert type V into a string
	ss<<val;
	string skey(key); // key may be a const char* or a string
	string sval(ss.str());

	// block so one thread can't write while another reads
	pthread_mutex_lock(&parameter_mutex);

	JParameter *p = GetParameterNoLock(skey);
	if(!p){
		p = new JParameter(skey, sval);
		parameters.push_back(p);
		p->type = JParameter::DataType(val);
	}else{
		p->SetValue(sval);
	}
	
	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);

	return p;
}

//---------------------------------
// GetParameterNoLock
//---------------------------------
template<typename K>
JParameter* JParameterManager::GetParameterNoLock(K key)
{
	/// This is the thread un-safe routine for getting a JParameter*.
	/// Un-safe is mis-leading since this actually exists to provide
	/// thread safety. This is called by both SetParameter() and
	/// GetParameter(). Both of those routines lock the parameters
	/// mutex while calling this one. That way, we guarantee that
	/// the parameters list is not modified while it is being read.
	/// The only drawback is that reads are also serialized possibly
	/// losing a little in efficiency when running multi-threaded. 
	
	string skey(key);
	vector<JParameter*>::iterator iter = parameters.begin();
	for(; iter!= parameters.end(); iter++){
		if((*iter)->GetKey() == skey){
			return *iter;
		}
	}

	return NULL;
}

//---------------------------------
// GetParameter
//---------------------------------
template<typename K>
JParameter* JParameterManager::GetParameter(K key)
{
	/// Thread safe call to get a JParameter*
	
	// block so one thread can't write while another reads
	pthread_mutex_lock(&parameter_mutex);

	JParameter *p = GetParameterNoLock(key);

	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);
	
	return p;
}

//---------------------------------
// GetParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::GetParameter(K key, V &val)
{
	JParameter *p = GetParameter(key);
	if(p){
		// use stringstream to convert string into V
		stringstream ss(p->GetValue());		
		ss>>val;
	}
	return p;
}

//---------------------------------
// GetParameter
//---------------------------------
template<typename K>
JParameter* JParameterManager::GetParameter(K key, string &val)
{
	// Strings are a special case in that they cn contain white space that we want
	// to copy. If we allow the fully templated version to handle strings,
	// then the stringstream operator will stop at the first white space
	// encountered, truncating the string.

	JParameter *p = GetParameter(key);
	if(p){
		val = p->GetValue();
	}
	return p;
}

} // Close JANA namespace


// Hide the following from rootcint
#ifndef __CINT__

// Global variable for accessing parameters (defined in JParameterManager.cc)
extern jana::JParameterManager *gPARMS;

#endif // __CINT__

#endif // _JParameterManager_

