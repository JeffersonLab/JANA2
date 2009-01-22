// $Id$
//
//    File: JCalibration.h
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#ifndef _JCalibration_
#define _JCalibration_

#include "jerror.h"

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

// Place everything in JANA namespace
namespace jana
{

class JCalibration{
	public:
		JCalibration(string url, int run, string context="default");
		virtual ~JCalibration();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JCalibration";}
		
		virtual bool GetCalib(string namepath, map<string, string> &svals)=0;
		virtual bool GetCalib(string namepath, vector< map<string, string> > &svals)=0;

		template<class T> bool Get(string namepath, map<string,T> &vals);
		template<class T> bool Get(string namepath, vector<T> &vals);
		template<class T> bool Get(string namepath, vector< map<string,T> > &vals);
		template<class T> bool Get(string namepath, vector< vector<T> > &vals);
		
		template<class T> bool Get(string namepath, const T* &vals);
		
		const int& GetRunRequested(void) const {return run_requested;}
		const int& GetRunFound(void) const {return run_found;}
		const int& GetRunMin(void) const {return run_min;}
		const int& GetRunMax(void) const {return run_max;}
		const string& GetContext(void) const {return context;}
		const string& GetURL(void) const {return url;}
		void GetAccesses(map<string, vector<string> > &accesses){accesses = this->accesses;}

	protected:
		int run_min;
		int run_max;
		int run_found;

	private:
		JCalibration(){} // Don't allow trivial constructor

		int run_requested;
		string context;
		string url;

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
bool JCalibration::Get(string namepath, map<string,T> &vals)
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
	bool res = GetCalib(namepath, svals);
	RecordRequest(namepath, typeid(T).name());
	
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
bool JCalibration::Get(string namepath, vector<T> &vals)
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
	map<string, string> svals;
	bool res = GetCalib(namepath, svals);
	RecordRequest(namepath, typeid(T).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	map<string,string>::const_iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); ++iter){
		// Use stringstream to convert from a string to type "T"
		T v;
		stringstream ss(iter->second);
		ss >> v;
		vals.push_back(v);
	}
	
	return res;
}

//-------------
// Get  (table, map version)
//-------------
template<class T>
bool JCalibration::Get(string namepath, vector< map<string,T> > &vals)
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
	bool res = GetCalib(namepath, svals);
	RecordRequest(namepath, typeid(T).name());
	
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
bool JCalibration::Get(string namepath, vector< vector<T> > &vals)
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
	vector< map<string, string> >svals;
	bool res = GetCalib(namepath, svals);
	RecordRequest(namepath, typeid(T).name());
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vals.clear();
	for(unsigned int i=0; i<svals.size(); i++){
		map<string,string>::const_iterator iter;
		vector<T> vvals;
		for(iter=svals[i].begin(); iter!=svals[i].end(); ++iter){
			// Use stringstream to convert from a string to type "T"
			T v;
			stringstream ss(iter->second);
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
bool JCalibration::Get(string namepath, const T* &vals)
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

	// Look to see if we already have this stored
	map<pair<string,string>, void*>::iterator iter = stored.find(key);
	if(iter!=stored.end()){
		vals = (const T*)iter->second;
		RecordRequest(namepath, typeid(T).name());
		return false; // return false to indicated success
	}


	// Looks like we don't have it stored already. Allocate memory for
	// the container and fill it with the constants.
	T* t = new T;
	bool res = Get(namepath, *t);
	
	// If successfull, store the pointer and copy it into the vals variable 
	if(!res){ // res==false means Get call was successful
		stored[key] = t;
		vals = t;
	}
	
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

	// vector<T>
	if(type_name==typeid(vector<T>).name()){
		delete (vector<T>*)ptr;
		return true;
	}

	// map<string,T>
	if(type_name==typeid(map<string,T>).name()){
		delete (map<string,T>*)ptr;
		return true;
	}

	// vector<vector<T> >
	if(type_name==typeid(vector<vector<T> >).name()){
		delete (vector<vector<T> >*)ptr;
		return true;
	}

	// vector<map<string,T> >
	if(type_name==typeid(vector<map<string,T> >).name()){
		delete (vector<map<string,T> >*)ptr;
		return true;
	}

	return false;
}

} // Close JANA namespace

#endif // _JCalibration_

