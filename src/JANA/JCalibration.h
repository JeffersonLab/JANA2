// $Id$
//
//    File: JCalibration.h
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#ifndef _JCalibration_
#define _JCalibration_

#include "jerror.h"

#include <typeinfo>
#include <stdint.h>
#include <pthread.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
using std::map;
using std::string;
using std::stringstream;
using std::vector;
using std::pair;

#include <JANA/JException.h>

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"

// Place everything in JANA namespace
namespace jana{

class JCalibration{
	public:
	
		enum containerType_t{
			kUnknownType,
			kVector,
			kMap,
			kVectorVector,
			kVectorMap
		};
	
		                       JCalibration(string url, int run, string context="default");
		               virtual ~JCalibration();
		   virtual const char* className(void){return static_className();}
		    static const char* static_className(void){return "JCalibration";}
		
		// Returns "false" on success and "true" on error
		          virtual bool GetCalib(string namepath, map<string, string> &svals, uint64_t event_number=0)=0;
		          virtual bool GetCalib(string namepath, vector<string> &svals, uint64_t event_number=0)=0;
		          virtual bool GetCalib(string namepath, vector< map<string, string> > &svals, uint64_t event_number=0)=0;
		          virtual bool GetCalib(string namepath, vector< vector<string> > &svals, uint64_t event_number=0)=0;
		          virtual bool PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string, string> &svals, string comment="");
		          virtual bool PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string, string> > &svals, string comment="");
		          virtual void GetListOfNamepaths(vector<string> &namepaths)=0;
		          virtual void GetEventBoundaries(vector<uint64_t> &event_boundaries); ///< User-callable access to event boundaries

		template<class T> bool Get(string namepath, map<string,T> &vals, uint64_t event_number=0);
		template<class T> bool Get(string namepath, vector<T> &vals, uint64_t event_number=0);
		template<class T> bool Get(string namepath, vector< map<string,T> > &vals, uint64_t event_number=0);
		template<class T> bool Get(string namepath, vector< vector<T> > &vals, uint64_t event_number=0);

		template<class T> bool Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string,T> &vals, const string &comment="");
		template<class T> bool Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector<T> &vals, const string &comment="");
		template<class T> bool Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string,T> > &vals, const string &comment="");
		template<class T> bool Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< vector<T> > &vals, const string &comment="");

		template<class T> bool Get(string namepath, const T* &vals, uint64_t event_number=0);
		
		       const int32_t& GetRun(void) const {return run_number;}
		         const string& GetContext(void) const {return context;}
		         const string& GetURL(void) const {return url;}
		                  void GetAccesses(map<string, vector<string> > &accesses){accesses = this->accesses;}
		                string GetVariation(void);
		
		       containerType_t GetContainerType(string typeid_name);
		                  void DumpCalibrationsToFiles(string basedir="./");
		                  void WriteCalibFileVector(string dir, string fname, string pathname);
		                  void WriteCalibFileMap(string dir, string fname, string pathname);
		                  void WriteCalibFileVectorVector(string dir, string fname, string pathname);
		                  void WriteCalibFileVectorMap(string dir, string fname, string pathname);

	protected:
		int32_t run_number;
		
		pthread_mutex_t accesses_mutex;
		pthread_mutex_t stored_mutex;
		pthread_mutex_t boundaries_mutex;

		template<typename T> containerType_t TrycontainerType(string typeid_name);

		// If the database supports event-level boundaries for calibration constants,
		// then the RetrieveEventBoundaries() method may be implemented by the 
		// subclass. It will be called automatically by GetEventBoundaries() when needed.
		// It will also be called while inside a mutex lock so there is no need to lock
		// a mutex inside this call. RetrieveEventBoundaries will be called exactly zero or 
		// one time for this JCalibration object. Values should be copied into the event_boundaries
		// container. The retrieved_event_boundaries flag will be updated automatically
		// so the subclass should not set it.
		virtual void RetrieveEventBoundaries(void){} ///< Optional for DBs that support event-level boundaries

	private:
		JCalibration(){} // Don't allow trivial constructor

		string context;
		string url;
		
		// Container to hold map of event boundaries. For (rare) cases when multiple sets
		// of calibration constants are needed for a single run, event-level boundaries can
		// be used.
		vector<uint64_t> event_boundaries;
		bool retrieved_event_boundaries; // Set automatically. Do NOT set this in the subclass

		// Container to hold all stored sets of constants. The "key" is a pair made from
		// the namepath and the typid().name() of the type stored. The value is a pointer
		// to the data object container itself.
		map<pair<string,string>, void*> stored;
		
		/// Attempt to delete the element in "stored" pointed to by iter.
		/// Return true if deleted, false if not.
		template<typename T> bool TryDelete(map<pair<string,string>, void*>::iterator iter);
		
		// Container to keep track of which constants were requested. The key is the
		// namepath and the value is a vector of typeid::name() strings of the data
		// types making the request. The vector may contain multiple instances of the
		// same type string so that the size of the vector is the total number of
		// accesses (probably a mulitple of the number of threads).
		map<string, vector<string> > accesses;

		/// Record a request for the calibration constants
		void RecordRequest(string namepath, string type_name);
};

//-------------
// Get  (map version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, map<string,T> &vals, uint64_t event_number)
{
	/// Templated method used to get a set of calibration constants.
	///
	/// This method will get the specified calibration constants in the form of
	/// strings using the virtual (non-templated) Get(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// The values are copied into <i>vals</i> using the keys it finds
	/// in the database, if any. If no keys are present, then numerical
	/// indices starting from zero are used. Note though that if non-keyed
	/// constants are used, then it may be more efficient for you to use
	/// the vector version of this method instead of the map one.
	
	// Get values in the form of strings
	map<string, string> svals;
	bool res = GetCalib(namepath, svals, event_number);
	RecordRequest(namepath, typeid(map<string,T>).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	map<string,string>::const_iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); ++iter){
		// Use stringstream to convert from a string to type "T"
		T v;
		stringstream ss(iter->second);
		ss >> v;
		vals[iter->first] = v;
	}
	
	return res;
}

//-------------
// Get  (vector version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, vector<T> &vals, uint64_t event_number)
{
	/// Templated method used to get a set of calibration constants.
	///
	/// This method will get the specified calibration constants in the form of
	/// strings using the virtual (non-templated) Get(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// The values are copied into <i>vals</i> in the order they are
	/// received from the virtual Get(...) method. If keys are returned
	/// with the data, they are discarded. Note though that if keyed
	/// constants are used, you may want to look at using
	/// the map version of this method instead of the vector one.
	
	// Get values in the form of strings
	vector<string> svals;
	bool res = GetCalib(namepath, svals, event_number);
	RecordRequest(namepath, typeid(vector<T>).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	vector<string>::const_iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); ++iter){
		// Use stringstream to convert from a string to type "T"
		T v;
		stringstream ss(*iter);
		ss >> v;
		vals.push_back(v);
	}
	
	return res;
}

//-------------
// Get  (table, map version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, vector< map<string,T> > &vals, uint64_t event_number)
{
	/// Templated method used to get a set of calibration constants.
	///
	/// This method will get the specified calibration constants in the form of
	/// strings using the virtual (non-templated) Get(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of the maps in <i>vals</i> are based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// This version of <i>Get</i> is used to read in data formatted as a
	/// table. The values are stored in a vector of maps with keys obtained
	/// either from the last comment line before the first line of data,
	/// or, if no such comment line exists, using the column number as
	/// the key. For example:
	///
	///<p><table border=1><TR><TD><tt>
	///# amp   mean  sigma
	///4.71  8.9  0.234
	///5.20  9.1  0.377
	///4.89  8.8  0.314 
	///</tt></TD></TR></table></p>
	///
	/// This would fill the vector <i>vals</i> with 3 elements. Each would be a map
	/// with 3 values using the keys "amp", "mean", and "sigma".
	/// To access them, use the syntax:
	///
	/// <p> vals[row][key] </p>
	///
	/// So, in the above example vals[0]["sigma"] would have the value 0.234 .
	/// 


	// Get values in the form of strings
	vector< map<string, string> >svals;
	bool res = GetCalib(namepath, svals, event_number);
	RecordRequest(namepath, typeid(vector< map<string,T> >).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	for(unsigned int i=0; i<svals.size(); i++){
		map<string,string>::const_iterator iter;
		map<string,T> mvals;
		for(iter=svals[i].begin(); iter!=svals[i].end(); ++iter){
			// Use stringstream to convert from a string to type "T"
			T v;
			stringstream ss(iter->second);
			ss >> v;
			mvals[iter->first] = v;
		}
		vals.push_back(mvals);
	}
	
	return res;
}

//-------------
// Get  (table, vector version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, vector< vector<T> > &vals, uint64_t event_number)
{
	/// Templated method used to get a set of calibration constants.
	///
	/// This method will get the specified calibration constants in the form of
	/// strings using the virtual (non-templated) Get(...) method. It will
	/// then convert the strings into the data type on which the inner vector
	/// is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// This version of <i>Get</i> is used to read in data formatted as a
	/// table. The values are stored in a vector of vectors with the inner
	/// vector representing a single row (i.e. one element for each column) and
	/// the outer vector collecting the rows. For example:
	///
	///<p><table border=1><TR><TD><tt>
	///# amp   mean  sigma
	///4.71  8.9  0.234
	///5.20  9.1  0.377
	///4.89  8.8  0.314 
	///</tt></TD></TR></table></p>
	///
	/// This would fill the vector <i>vals</i> with 3 elements. Each would be a vector
	/// with 3 values. To access them, use the syntax:
	///
	/// <p> vals[row][column] </p>
	///
	/// So, in the above example vals[0][2] would have the value 0.234 .
	/// 
	
	// Get values in the form of strings
	vector< vector<string> >svals;
	bool res = GetCalib(namepath, svals, event_number);
	RecordRequest(namepath, typeid(vector< vector<T> >).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	for(unsigned int i=0; i<svals.size(); i++){
		vector<string>::const_iterator iter;
		vector<T> vvals;
		for(iter=svals[i].begin(); iter!=svals[i].end(); ++iter){
			// Use stringstream to convert from a string to type "T"
			T v;
			stringstream ss(*iter);
			ss >> v;
			vvals.push_back(v);
		}
		vals.push_back(vvals);
	}
	
	return res;
}

//-------------
// Get  (stored container version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, const T* &vals, uint64_t event_number)
{
	/// Templated method used to get a set of calibration constants.
	///
	/// Get a pointer to the specified set of constants but keep them
	/// in the JCalibration object. If the specified constants have already
	/// been retrieved using type T, then the pointer is copied and the
	/// routine returns immediately. Otherwise, the constants are retrieved
	/// using one of the other Get() methods and stored locally before returning
	/// a pointer so subsequent calls will get the same pointer.
	
	// Initialize return pointer to reasonable value
	vals = NULL;
	
	// Create key from namepath and data type
	pair<string, string> key;
	key.first = namepath;
	key.second = typeid(T).name();
	
	// Lock mutex while accessing stored data
	pthread_mutex_lock(&stored_mutex);

	// Look to see if we already have this stored
	map<pair<string,string>, void*>::iterator iter = stored.find(key);
	if(iter!=stored.end()){
		vals = (const T*)iter->second;
		pthread_mutex_unlock(&stored_mutex);
		RecordRequest(namepath, typeid(T).name());
		return false; // return false to indicated success
	}


	// Looks like we don't have it stored already. Allocate memory for
	// the container and fill it with the constants.
	T* t = new T;
	bool res = Get(namepath, *t, event_number);
	
	// If successfull, store the pointer and copy it into the vals variable 
	if(!res){ // res==false means Get call was successful
		stored[key] = t;
		vals = t;
	}
	
	// Release stored mutex
	pthread_mutex_unlock(&stored_mutex);
	
	return res;
}

//-------------
// Put  (map version)
//-------------
template<class T>
bool JCalibration::Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string,T> &vals, const string &comment)
{
	/// Templated method used to write a set of calibration constants.
	///
	/// This method will write the specified calibration constants in the form of
	/// strings using the virtual (non-templated) PutCalib(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	
	// Loop over values, converting the type "T" values to strings 
	map<string,string> svals;
	//map<string,T>::const_iterator iter;
	__typeof__(vals.begin()) iter;
	for(iter=vals.begin(); iter!=vals.end(); ++iter){
		// Use stringstream to convert from a string to type "T"
		stringstream ss;
		ss<<iter->second;
		svals[iter->first] = ss.str();
	}

	// Put values that have been converted to strings
	bool res = PutCalib(namepath, run_min, run_max, event_min, event_max, author, svals, comment);
	
	return res;
}

//-------------
// Put  (vector version)
//-------------
template<class T>
bool JCalibration::Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector<T> &vals, const string &comment)
{
	/// Templated method used to write a set of calibration constants.
	///
	/// This method will write the specified calibration constants in the form of
	/// strings using the virtual (non-templated) PutCalib(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	
	// Loop over values, converting the type "T" values to strings 
	map<string,string> svals;
	__typeof__(vals.begin()) iter;
	int i=0;
	for(iter=vals.begin(); iter!=vals.end(); ++iter, ++i){
		// Use stringstream to convert from a string to type "T"
		stringstream ss;
		ss<<iter->second;
		stringstream iss;
		iss<<i;
		svals[iss.str()] = ss.str();
	}

	// Put values that have been converted to strings
	bool res = PutCalib(namepath, run_min, run_max, event_min, event_max, author, svals, comment);
	
	return res;
}

//-------------
// Put  (table, map version)
//-------------
template<class T>
bool JCalibration::Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string,T> > &vals, const string &comment)
{
	/// Templated method used to write a set of calibration constants.
	///
	/// This method will write the specified calibration constants in the form of
	/// strings using the virtual (non-templated) PutCalib(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	
	// Loop over vector
	vector<map<string,string> > vsvals;
	for(unsigned int i=0; i<vals.size(); i++){
		// Loop over values, converting the type "T" values to strings 
		map<string,string> svals;
		map<string, T> &mvals = vals[i];
		__typeof__(mvals.begin()) iter;
		for(iter=mvals.begin(); iter!=mvals.end(); ++iter){
			// Use stringstream to convert from a string to type "T"
			stringstream ss;
			ss<<iter->second;
			svals[iter->first] = ss.str();
		}
		vsvals.push_back(svals);
	}

	// Put values that have been converted to strings
	bool res = PutCalib(namepath, run_min, run_max, event_min, event_max, author, vsvals, comment);
	
	return res;
}
	
//-------------
// Put  (table, vector version)
//-------------
template<class T>
bool JCalibration::Put(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< vector<T> > &vals, const string &comment)
{
	/// Templated method used to write a set of calibration constants.
	///
	/// This method will write the specified calibration constants in the form of
	/// strings using the virtual (non-templated) PutCalib(...) method. It will
	/// then convert the strings into the data type on which the "value"
	/// part of <i>vals</i> is based. It does this using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	
	// Loop over vector
	vector<map<string,string> > vsvals;
	for(unsigned int i=0; i<vals.size(); i++){
		// Loop over values, converting the type "T" values to strings 
		map<string,string> svals;
		vector<T> &mvals = vals[i];
		for(unsigned int j=0; j<mvals.size(); j++){
			// Use stringstream to convert from a string to type "T"
			stringstream ss;
			ss<<mvals[j];
			stringstream iss;
			iss<<i;
			svals[iss.str()] = ss.str();
		}
		vsvals.push_back(svals);
	}

	// Put values that have been converted to strings
	bool res = PutCalib(namepath, run_min, run_max, event_min, event_max, author, vsvals, comment);
	
	return res;
}

//-------------
// TryDelete
//-------------
template<typename T>
bool JCalibration::TryDelete(map<pair<string,string>, void*>::iterator iter)
{
	/// Attempt to delete the element in "stored" pointed to by iter.
	/// Return true if deleted, false if not.
	///
	/// This method is maily called from the JCalibration destructor.
	const string &type_name = iter->first.second;
	void *ptr = iter->second;

	switch(TrycontainerType<T>(type_name)){
		case kVector:			delete (vector<T>*)ptr;						break;
		case kMap:				delete (map<string,T>*)ptr;				break;
		case kVectorVector:	delete (vector<vector<T> >*)ptr;			break;
		case kVectorMap:		delete (vector<map<string,T> >*)ptr;	break;
		default:
			// Type T must not be the right type. Inform caller
			return false;
	}

	// If we get here, then we must have found the type above and deleted the container
	return true;
}

//-------------
// TrycontainerType
//-------------
template<typename T>
JCalibration::containerType_t JCalibration::TrycontainerType(string typeid_name)
{
	if(typeid_name==typeid(vector<T>).name())return kVector;
	if(typeid_name==typeid(map<string,T>).name())return kMap;
	if(typeid_name==typeid(vector<vector<T> >).name())return kVectorVector;
	if(typeid_name==typeid(vector<map<string,T> >).name())return kVectorMap;

	return kUnknownType;
}

} // Close JANA namespace

#endif // _JCalibration_

