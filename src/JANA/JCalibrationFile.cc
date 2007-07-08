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

//---------------------------------
// JCalibrationFile    (Constructor)
//---------------------------------
JCalibrationFile::JCalibrationFile(string url, int run, string context):JCalibration(url,run,context)
{
	// File URL should be of form:
	// file:///path-to-root-dir
	// The value of "context" is assumed to be the name of a directory inside the
	// calibration root directory. The value of namepath will then be followed
	// relative to that.

	// First, check that the URL even starts with "file://"
	if(url.substr(0, 7)!=string("file://")){
		_DBG_<<"Poorly formed URL. Should start with \"file://\"."<<endl;
		_DBG_<<"URL:"<<url<<endl;
		return;
	}
	
	// Fill basedir with path to directory.
	basedir = url;
	basedir.replace(0,7,string("")); // wipe out "file://" part
	basedir += "/" + context + "/";

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
	// this calibration is valid. For now, just set them all to run_requested.
	run_min = run_max = run_found = GetRunRequested();
	
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
// Get
//---------------------------------
bool JCalibrationFile::Get(string namepath, map<string, string> &svals)
{
	/// Open file specified by namepath (and the url passed to us in the
	/// constructor) and read in the calibration constants in plain
	/// ascii from it. Values are copied into <i>vals</i>.
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

