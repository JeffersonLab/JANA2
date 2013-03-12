// $Id$
//
//    File: JGeometry.h
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#ifndef _JGeometry_
#define _JGeometry_

#include "jerror.h"

#include <map>
#include <string>
#include <sstream>
#include <vector>
using std::map;
using std::string;
using std::stringstream;
using std::vector;

// Place everything in JANA namespace
namespace jana{

/// JGeometry is a virtual base class used to define the interface by
/// which geometry information can be obtained in JANA.
/// Implementing this base class allows the JANA end user to be
/// agnostic as to the details of how the geometry info is stored.
/// The geometry can be stored in a database or any number of file
/// formats. The files can be stored locally, or on the network
/// somewhere.
///
/// The primary advantage here is that it allows one to work with
/// local files, but then easily switch to a remote source method
/// when appropriate without requiring modifications to the end
/// user code.
///
/// On the user side they will call one of the Get(...) methods which all get
/// translated into a call of one of the two vitural methods:
///
///   virtual bool Get(string namepath, string &sval, map<string, string> &where)=0;
///   virtual bool Get(string namepath, map<string, string> &svals, map<string, string> &where)=0;
///
/// These two virtual methods along with one to get a list of the available
/// namepaths are the only things that need to be implemented
/// in a concrete subclass of JGeometry.
///
/// A geometry element is specified by its <i>namepath</i> and an optional
/// set of qualifiers (the <i>where</i> argument). The <i>namepath</i>
/// is a hierarchal list of elements separated by forward slashes(/)
/// analogous to a path on a unix filesystem. This path is always assumed
/// to be relative to the <i>url</i> specified in the constructor. So,
/// for instance, suppose one kept the geometry in a set XML files on the
/// local filesystem and wished to access information from the file
///
/// <tt>/home/joe/calib/geom_Oct10_2017.xml</tt>
///
/// One would specify the <i>url</i> as:
///
/// file:///home/joe/calib/geom_Oct10_2017.xml
///
/// and then the namepath could be specified as the string:
///
/// "TOF/bar/X_Y_Z"
///
/// which would indicate the attribute <i>"X_Y_Z"</i> of the
/// subtag <i>"bar"</i> of the tag <i>"TOF"</i> in the file
/// <i>"/home/joe/calib/geom_Oct10_2017.xml"</i>

class JGeometry{
	public:
		JGeometry(string url, int run, string context="default"){
			this->url = url;
			this->run_requested = run;
			this->context = context;
		}
		virtual ~JGeometry(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JGeometry";}
		
		typedef enum{
			// Used to specify which (if any) attributes should be included in
			// the values obtained through GetXPaths().
			attr_level_none = 0,	// Don't include any attributes. Only node names.
			attr_level_last = 1,	// Include the attributes for the last node only.
			attr_level_all  = 2	// Include attributes for all nodes.
		}ATTR_LEVEL_t;
		
		// Virtual methods called through base class
		virtual bool Get(string xpath, string &sval)=0;
		virtual bool Get(string xpath, map<string, string> &svals)=0;
		virtual bool GetMultiple(string xpath, vector<string> &vsval)=0;
		virtual bool GetMultiple(string xpath, vector<map<string, string> >&vsvals)=0;
		virtual void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level=attr_level_last, const string &filter="")=0;
		virtual string GetChecksum(void) const {return string("not supported");}

		// Templated methods that can return more useful forms
		template<class T> bool Get(string xpath, T &val);
		template<class T> bool Get(string xpath, vector<T> &vals, string delimiter=" ");
		template<class T> bool Get(string xpath, map<string,T> &vals);
		template<class T> bool GetMultiple(string xpath, vector<T> &vval);
		template<class T> bool GetMultiple(string xpath, vector<vector<T> > &vvals, string delimiter=" ");
		template<class T> bool GetMultiple(string xpath, vector<map<string,T> > &vvals);
		
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
		JGeometry(){} // Don't allow trivial constructor

		int run_requested;
		string context;
		string url;
		map<string,string> anywhere; // an empty map means no constraints

};


//-------------
// Get  (single version)
//-------------
template<class T>
bool JGeometry::Get(string xpath, T &val)
{
	/// Templated method used to get a single geometry element.
	///
	/// This method will get the specified geometry element in the form of
	/// a string using the virtual (non-templated) Get(...) method. It will
	/// then convert the string into the data type on which <i>val</i> is
	/// based. It does this using the stringstream
	/// class so T is restricted to the types stringstream understands (int, float, 
	/// double, string, ...).
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of a string
	string sval;
	bool res = Get(xpath, sval);
	if(!res)return res;
	
	// Convert the string to type "T" and copy it into val.
	// Use stringstream to convert from a string to type "T"
	stringstream ss(sval);
	ss >> val;
	
	return res;
}

//-------------
// Get  (vector version)
//-------------
template<class T>
bool JGeometry::Get(string xpath, vector<T> &vals, string delimiter)
{
	/// Templated method used to get a set of values from a geometry attribute.
	///
	/// This method can be used to get a list of values (possibly only one)
	/// from a single geometry attribute specified by xpath. The attribute
	/// is obtained as a string using the non-templated Get(...) method
	/// and the string broken into tokens separated by the delimiter
	/// (which defaults to a single white space). Each
	/// token is then converted into type T using the stringstream class
	/// so T is restricted to the types stringstream understands (int, float, 
	/// double, string, ...).
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of strings
	vals.clear();
	string svals;
	bool res = Get(xpath, svals);
	if(!res)return res;
	
	string::size_type pos_start = svals.find_first_not_of(delimiter,0);
	while(pos_start != string::npos){
		string::size_type pos_end = svals.find_first_of(delimiter, pos_start);
		if(pos_end==string::npos)pos_end=svals.size();

		T v;
		string val = svals.substr(pos_start, pos_end-pos_start);
		stringstream ss(val);
		ss >> v;
		vals.push_back(v);
		
		pos_start = svals.find_first_not_of(delimiter, pos_end);
	}

	return res;
}

//-------------
// Get  (map version)
//-------------
template<class T>
bool JGeometry::Get(string xpath, map<string,T> &vals)
{
	/// Templated method used to get a set of geometry attributes.
	///
	/// This method can be used to get a list of all attributes for
	/// a given xpath. The attributes are copied into the <i>vals</i>
	/// map with the attribute name as the key and the attribute
	/// value as the value. This relies on the non-templated, virtual
	/// Get(string, map<string,string>&) method to first get the values
	/// in the form of strings. It converts them using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of strings
	map<string, string> svals;
	bool res = Get(xpath, svals);
	
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
// GetMultiple  (single version)
//-------------
template<class T>
bool JGeometry::GetMultiple(string xpath, vector<T> &vval)
{
	/// Templated method used to get multiple entries satisfying a single xpath.
	///
	/// This method will get the specified geometry element in the form of
	/// a string using the virtual (non-templated) Get(...) method. It will
	/// then convert the string into the data type on which <i>val</i> is
	/// based. It does this using the stringstream
	/// class so T is restricted to the types stringstream understands (int, float, 
	/// double, string, ...).
	///
	/// This differs from the similar Get() method in that the geometry tree
	/// will be searched for all nodes satisfying the given xpath and all
	/// all values will be copied into the container provided. In Get(), only
	/// the first node encountered that satisfies the xpath will be copied.
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of a string
	vector<string> vsval;
	bool res = GetMultiple(xpath, vsval);
	if(!res)return res;
	
	// Convert the string to type "T" and copy it into val.
	// Use stringstream to convert from a string to type "T"
	for(unsigned int i=0; i<vsval.size(); i++){
		stringstream ss(vsval[i]);
		T val;
		ss >> val;
		vval.push_back(val);
	}
	
	return res;
}

//-------------
// GetMultiple  (vector version)
//-------------
template<class T>
bool JGeometry::GetMultiple(string xpath, vector<vector<T> > &vvals, string delimiter)
{
	/// Templated method used to get a set of values from a geometry attribute.
	///
	/// This method can be used to get a list of values (possibly only one)
	/// from a single geometry attribute specified by xpath. The attribute
	/// is obtained as a string using the non-templated Get(...) method
	/// and the string broken into tokens separated by the delimiter
	/// (which defaults to a single white space). Each
	/// token is then converted into type T using the stringstream class
	/// so T is restricted to the types stringstream understands (int, float, 
	/// double, string, ...).
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of strings
	vvals.clear();
	vector<string> vsvals;
	bool res = GetMultiple(xpath, vsvals);
	if(!res)return res;

	for(unsigned int i=0; i<vsvals.size(); i++){
		string &svals = vsvals[i];

		string::size_type pos_start = svals.find_first_not_of(delimiter,0);
		vector<T> vals;
		while(pos_start != string::npos){
			string::size_type pos_end = svals.find_first_of(delimiter, pos_start);
			if(pos_end==string::npos)pos_end=svals.size();

			T v;
			string val = svals.substr(pos_start, pos_end-pos_start);
			stringstream ss(val);
			ss >> v;
			vals.push_back(v);
			
			pos_start = svals.find_first_not_of(delimiter, pos_end);
		}
		
		vvals.push_back(vals);
	}

	return res;
}

//-------------
// GetMultiple  (map version)
//-------------
template<class T>
bool JGeometry::GetMultiple(string xpath, vector<map<string,T> > &vvals)
{
	/// Templated method used to get a set of geometry attributes.
	///
	/// This method can be used to get a list of all attributes for
	/// a given xpath. The attributes are copied into the <i>vals</i>
	/// map with the attribute name as the key and the attribute
	/// value as the value. This relies on the non-templated, virtual
	/// Get(string, map<string,string>&) method to first get the values
	/// in the form of strings. It converts them using the stringstream
	/// class so T is restricted to the types it understands (int, float, 
	/// double, string, ...).
	///
	/// If no element of the specified name is found, a value
	/// of boolean "false" is returned. A value of "true" is 
	/// returned upon success.
	
	// Get values in the form of strings
	vector<map<string, string> > vsvals;
	bool res = GetMultiple(xpath, vsvals);
	
	// Loop over values, converting the strings to type "T" and
	// copying them into the vals map.
	vvals.clear();

	for(unsigned int i=0; i<vsvals.size(); i++){
		map<string,string> &svals = vsvals[i];
	
		map<string,T> vals;
		map<string,string>::const_iterator iter;
		for(iter=svals.begin(); iter!=svals.end(); ++iter){
			// Use stringstream to convert from a string to type "T"
			T v;
			stringstream ss(iter->second);
			ss >> v;
			vals[iter->first] = v;
		}
		
		vvals.push_back(vals);
	}

	return res;
}

} // Close JANA namespace

#endif // _JGeometry_
