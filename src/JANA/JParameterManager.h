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

#include "JParameter.h"

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
		template<typename K> JParameter* GetParameter(K key); ///< Get the value of a configuration parameter
		template<typename K, typename V> JParameter* GetParameter(K key, V &val); ///< Get pointer to configuration parameters JParameter object
		void GetParameters(map<string,string> &parms, string filter="");
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
	V my_val = val;
	
	JParameter *p = GetParameter(key,val);
	if(!p){
		p = SetParameter(key, val);
		p->isdefault = true;
	}else{
		// Warn user if two different default values are set
		if(p->isdefault){
			stringstream ss;
			ss<<val;
			if(ss.str() != p->GetValue()){
				std::cout<<" WARNING: Multiple calls to SetDefaultParameter with key=\""
				<<key<<"\" value= "<<val<<" and "<<my_val<<std::endl;
			}
		}else{
			// Parameters set from the command line will have the
			// wrong data type since SetParameter will have been called
			// with a string type for the value. If a default was set,
			// already for this parameter, then we don't need to set it
			// again, but if not, we should set it to the correct type.
			p->type = JParameter::DataType(val);
		}
	}
	
	// Set the "hasdefault" flag so typos can be filtered later
	p->hasdefault = true;

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
	}else{
		p->SetValue(sval);
	}
	p->isdefault = false;
	p->type = JParameter::DataType(val);
	
	// Tell the JParameterManager it needs to re-print if requested
	printParametersCalled = false;

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

} // Close JANA namespace


// Hide the following from rootcint
#ifndef __CINT__

// Global variable for accessing parameters (defined in JParameterManager.cc)
extern jana::JParameterManager *gPARMS;

#endif // __CINT__

#endif // _JParameterManager_

