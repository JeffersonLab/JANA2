// $Id$
//
//    File: JCalibrationFile.cc
// Created: Fri Jul  6 19:45:57 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#include <iostream>
#include <fstream>
using namespace std;

#include "JCalibrationFile.h"
using namespace jana;

//---------------------------------
// JCalibrationFile    (Constructor)
//---------------------------------
JCalibrationFile::JCalibrationFile(string url, int run, string context):JCalibration(url,run,context)
{
	// File URL should be of form:
	// file:///path-to-root-dir
	// The value of namepath will then be followed relative to this.
	
	// NOTE: The original design here added an additional "context" directory
	// that was always appended to the URL. This idea was shouted down at
	// the July 17, 2007 software meeting and so was removed.

	// First, check that the URL even starts with "file://"
	if(url.substr(0, 7)!=string("file://")){
		_DBG_<<"Poorly formed URL. Should start with \"file://\"."<<endl;
		_DBG_<<"URL:"<<url<<endl;
		return;
	}
	
	// Fill basedir with path to directory.
	basedir = url;
	basedir.replace(0,7,string("")); // wipe out "file://" part
	if(basedir[basedir.size()-1]!='/')basedir += "/";
	//basedir += context + "/"; // (see not above)

	// Try and get a hold of the info.xml file in basedir
	string fname = basedir + "info.xml";
	ifstream f(fname.c_str());
	if(!f.is_open()){
		_DBG_<<"Unable to open \""<<fname<<"\"!"<<endl;
	}
	
	// At this point, we need to read in the info file and probably
	// pass it to some standard parser in the JCalibration base class.
	// Any volunteers?
	// Among other things, this should contain the range of runs for which
	// this calibration is valid. For now, just set the found run to run_requested.
	run_found = GetRunRequested();
	run_min = 1;
	run_max = 10000;
	
	// Close info file
	f.close();
}

//---------------------------------
// ~JCalibrationFile    (Destructor)
//---------------------------------
JCalibrationFile::~JCalibrationFile()
{

}

//---------------------------------
// GetCalib
//---------------------------------
bool JCalibrationFile::GetCalib(string namepath, map<string, string> &svals)
{
	/// Open file specified by namepath (and the url passed to us in the
	/// constructor) and read in the calibration constants in plain
	/// ascii from it. Values are copied into <i>svals</i>.
	///
	/// File format can be one of two forms. Either one value per line
	/// or one key/value pair per line. If only one value is found, 
	/// the line number is used as the key. When both key and value
	/// are present, they must be separated by white space.
	/// Lines starting with # are considered comments and are ignored.
	
	// Clear svals map.
	svals.clear();
	
	// Open file
	string fname = basedir + namepath;
	ifstream f(fname.c_str());
	if(!f.is_open()){
		_DBG_<<"Unable to open \""<<fname<<"\"!"<<endl;
		return true;
	}
	
	// Loop over all lines in the file
	int nvals = 0;
	string line;
	while(getline(f, line, '\n')){
		if(line.length()==0)continue;
		if(line[0] == '#')continue;
		
		// Try and break up the line into a key and a value.
		stringstream ss(line);
		string key, val;
		ss>>key;
		if(key==line){
			// Must be just one item here. Copy it to val and set key to "nvals"
			val = key;
			stringstream sss;
			sss<<nvals; // This tricky bit is just to convert from an int to a string
			sss>>key;
		}else{
			// Looks like there is a key and a value here.
			ss>>val;
		}
		
		// Add this key value pair to svals.
		svals[key]=val;
		
		nvals++;
	}
	
	// Close file
	f.close();

	return false;
}

//---------------------------------
// GetCalib
//---------------------------------
bool JCalibrationFile::GetCalib(string namepath, vector< map<string, string> > &svals)
{
	/// Open file specified by namepath (and the url passed to us in the
	/// constructor) and read in a table of calibration constants in plain
	/// ascii. Values are copied into <i>svals</i>.
	///
	/// File format must be in a table form with the same number of items
	/// on every line. Items should be separated by white space.
	/// Lines starting with # are considered comments and are ignored
	/// except for the last commented line before the first line containing
	/// data. This is parsed (if present) to determine the column names
	/// that will be used to index the map. If an appropriate column
	/// names line is not found, numerical column names will be assigned
	/// starting from zero.
	
	// Clear svals map.
	svals.clear();
	
	// Open file
	string fname = basedir + namepath;
	ifstream f(fname.c_str());
	if(!f.is_open()){
		_DBG_<<"Unable to open \""<<fname<<"\"!"<<endl;
		return true;
	}
	
	// Loop over all lines in the file
	string colnamesline;
	vector<string> colnames;
	string line;
	while(getline(f, line, '\n')){
		if(line.length()==0)continue;
		if(line.substr(0,2) == "#%"){
			// Parse special comment line with column names
			string s(line);
			s.erase(0,2); // remove leading "#%" 
			stringstream sss(s);
			colnames.clear();
			string colname;
			while(sss>>colname)colnames.push_back(colname);
		}
		if(line[0] == '#')continue;

		// Break up the line into individual values and create a map indexed by column names
		stringstream ss(line);
		string val;
		map<string, string> mval;
		unsigned int icol = 0;
		while(ss>>val){
			// Make sure a name exists for this column
			if(colnames.size()<=icol){
				stringstream sss;
				sss<<icol;
				colnames.push_back(sss.str());
			}
			mval[colnames[icol++]] = val;
		}
		
		// Push row onto vector
		svals.push_back(mval);
	}
	
	// Close file
	f.close();

	// Check that all rows have the same number of columns
	unsigned int ncols = svals[0].size();
	for(unsigned int i=1; i<svals.size(); i++){
		if(svals[i].size() != ncols){
			_DBG_<<"Number of columns not the same for all rows in "<<fname<<endl;
			return true;
		}
	}
	
	return false;
}

