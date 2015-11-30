// $Id: JParameterManager.h 1230 2005-08-23 13:17:11Z davidl $
//
//    File: JParameterManager.h
// Created: Tue Aug 16 14:30:24 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JParameterManager_
#define _JParameterManager_

#include <iostream>
#include <iomanip>
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
#include <JANA/JException.h>

#include "jerror.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"


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
	
                template<typename K> JParameter* CreateParameter(K key, string description="");
		               template<typename K> bool Exists(K key);
    template<typename K, typename V> JParameter* SetDefaultParameter(K key, V& val, string description="");
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
		                                    void DumpSuccinct(bool print_descriptions=true); ///< Print one line for each param. optionally including descriptions
									        void SetVerbose(bool verbose=true){this->verbose = verbose;} ///< Turn on additional messages
		
	private:
		vector<JParameter*> parameters;
		pthread_mutex_t parameter_mutex;
		bool printParametersCalled;
		bool verbose;
};

	
//---------------------------------
// CreateParameter
//---------------------------------
template<typename K>
JParameter* JParameterManager::CreateParameter(K key, string description)
{
	/// Create a configuration parameter, throwing an exception if it already exists.
	///
	/// This will attempt to create a new Configuration parameter with the given
	/// name and description (if provided). If a parameter with that name already
	/// exists, then a JException is thrown.
	///
	/// Before using this, you should consider using SetDefaultParameter. That will
	/// allow the parameter to be changed from the command line or configuration file
	/// whereas this will create parameters that can only be modified programmatically.
	
	string skey(key); // in case key is a const char *

	// Lock mutex to make sure other threads don't change parameters on us
	pthread_mutex_lock(&parameter_mutex);
	
	// Get the parameter
	JParameter *p = GetParameterNoLock(key);

	if(p == NULL){
		// Create the parameter
		string sval("");
		p = new JParameter(skey, sval);
		parameters.push_back(p);
	}else{
		// If we get here then the parameter must already exist. Throw an exception.
		pthread_mutex_unlock(&parameter_mutex); // make sure we unlock mutex before leaving!
		string mess = "Parameter \""+skey+"\" already exists!";
		jerr << mess << std::endl;
		throw JException(mess);
	}
	
	// Unlock mutex
	pthread_mutex_unlock(&parameter_mutex);
	
	return p;
}

//---------------------------------
// Exists
//---------------------------------
template<typename K>
bool JParameterManager::Exists(K key)
{
	/// Check to see if a parameter with the given name exists.
	/// A boolean true is returned if the parameter exists and
	/// false if it does not.
	
	string skey(key); // in case key is a const char *

	// Lock mutex to make sure other threads don't change parameters on us
	pthread_mutex_lock(&parameter_mutex);
	JParameter *p = GetParameterNoLock(key);
	pthread_mutex_unlock(&parameter_mutex);
	
	return p != NULL;
}

//---------------------------------
// SetDefaultParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::SetDefaultParameter(K key, V &val, string description)
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
	/// To have this print a message to jerr if the requested parameter
	/// does not exist, set the verbose flag to true via
	/// the SetVerbose() method.
	
	// block so one thread can't write while another reads
	pthread_mutex_lock(&parameter_mutex);

	JParameter *p = GetParameterNoLock(key);

	// release the parameters mutex
	pthread_mutex_unlock(&parameter_mutex);
	
	// If parameter does not exist, thrown an exception
	if(!p){
		string mess = "Parameter does not exist: " + string(key);
		if(verbose) jerr << mess << std::endl;
		throw JException(mess);
	}
	
	return p;
}

//---------------------------------
// GetParameter
//---------------------------------
template<typename K, typename V>
JParameter* JParameterManager::GetParameter(K key, V &val)
{
	try{
		JParameter *p = GetParameter(key);
		// use stringstream to convert string into V
		stringstream ss(p->GetValue());		
		ss>>val;
		return p;
	}catch(exception &e){
		throw  e; // rethrow exception
		return NULL;
	}
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

	try{
		JParameter *p = GetParameter(key);
		val = p->GetValue();
		return p;
	}catch(exception &e){
		throw  e; // rethrow exception
		return NULL;
	}
}

} // Close JANA namespace


// Hide the following from rootcint
#if !defined(__CINT__) && !defined(__CLING__)

// Global variable for accessing parameters (defined in JParameterManager.cc)
extern jana::JParameterManager *gPARMS;

#endif // __CINT__  __CLING__

#endif // _JParameterManager_

