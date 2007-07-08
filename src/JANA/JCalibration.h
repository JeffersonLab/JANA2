// $Id$
//
//    File: JCalibration.h
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#ifndef _JCalibration_
#define _JCalibration_

#include "jerror.h"

#include <map>
#include <string>
#include <sstream>
#include <vector>
using std::map;
using std::string;
using std::stringstream;
using std::vector;

class JCalibration{
	public:
		JCalibration(string url, int run, string context="default");
		virtual ~JCalibration();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JCalibration";}
		
		template<class T> bool Get(string namepath, map<string,T> &vals);
		template<class T> bool Get(string namepath, vector<T> &vals);
		virtual bool Get(string namepath, map<string, string> &svals)=0;
		
		const int& GetRunRequested(void) const {return run_requested;}
		const int& GetRunFound(void) const {return run_found;}
		const int& GetRunMin(void) const {return run_min;}
		const int& GetRunMax(void) const {return run_max;}
		const string& GetContext(void) const {return context;}
		const string& GetURL(void) const {return url;}

	protected:
		int run_min;
		int run_max;
		int run_found;
	
	private:
		JCalibration(){} // Don't allow trivial constructor

		int run_requested;
		string context;
		string url;

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
	bool res = Get(namepath, svals);
	
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
	bool res = Get(namepath, svals);
	
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


#endif // _JCalibration_

