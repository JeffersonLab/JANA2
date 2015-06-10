// $Id$
//
//    File: JCalibrationFile.cc
// Created: Fri Jul  6 19:45:57 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;

#include "JCalibrationFile.h"
#include "JStreamLog.h"
using namespace jana;

//---------------------------------
// JCalibrationFile    (Constructor)
//---------------------------------
JCalibrationFile::JCalibrationFile(string url, int32_t run, string context):JCalibration(url,run,context)
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
		jout<<"WARNING: Unable to open \""<<fname<<"\" (not necessarily a problem)"<<endl;
	}
	
	// At this point, we need to read in the info file and probably
	// pass it to some standard parser in the JCalibration base class.
	// Any volunteers?
	// Among other things, this should contain the range of runs for which
	// this calibration is valid. For now, just set the found run to run_requested.
	run_number = GetRun(); // redunant, yes, but it's just a place holder
	
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
bool JCalibrationFile::GetCalib(string namepath, map<string, string> &svals, uint64_t event_number)
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
		string mess = "Unable to open \"" + fname + "\"!";
		throw JException(mess);
		return true; // never gets to this line
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
			// colum names are set to position with zero padding so map maintains
			// correct order (up to 9999)
			val = key;
			stringstream sss;
			sss << setw(4) << setfill('0') << nvals; // This tricky bit is just to convert from an int to a string
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
bool JCalibrationFile::GetCalib(string namepath, vector<string> &svals, uint64_t event_number)
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
	
	// n.b. this was copied from the above and only the "push_back" line modified
	// to ignore the key and store only the values in the order they are encountered
	
	// Clear svals map.
	svals.clear();
	
	// Open file
	string fname = basedir + namepath;
	ifstream f(fname.c_str());
	if(!f.is_open()){
		string mess = "Unable to open \"" + fname + "\"!";
		throw JException(mess);
		return true; // never gets to this line
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
			// colum names are set to position with zero padding so map maintains
			// correct order (up to 9999)
			val = key;
			stringstream sss;
			sss << setw(4) << setfill('0') << nvals; // This tricky bit is just to convert from an int to a string
			sss>>key;
		}else{
			// Looks like there is a key and a value here.
			ss>>val;
		}
		
		// Add this key value pair to svals.
		svals.push_back(val);
		
		nvals++;
	}
	
	// Close file
	f.close();

	return false;
}

//---------------------------------
// GetCalib
//---------------------------------
bool JCalibrationFile::GetCalib(string namepath, vector< map<string, string> > &svals, uint64_t event_number)
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
		string mess = "Unable to open \"" + fname + "\"!";
		throw JException(mess);
		return true; // never gets to this line
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
				sss << setw(4) << setfill('0') << icol;
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

//---------------------------------
// GetCalib
//---------------------------------
bool JCalibrationFile::GetCalib(string namepath, vector< vector<string> > &svals, uint64_t event_number)
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

	// n.b. this was copied from the above and only the minimal changes made
	// to ignore the key and store only the values in the order they are encountered
	
	// Clear svals map.
	svals.clear();
	
	// Open file
	string fname = basedir + namepath;
	ifstream f(fname.c_str());
	if(!f.is_open()){
		string mess = "Unable to open \"" + fname + "\"!";
		throw JException(mess);
		return true; // never gets to this line
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
		vector<string> mval;
		unsigned int icol = 0;
		while(ss>>val){
			// Make sure a name exists for this column
			if(colnames.size()<=icol){
				stringstream sss;
				sss << setw(4) << setfill('0') << icol;
				colnames.push_back(sss.str());
			}
			mval.push_back(val);
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

//---------------------------------
// PutCalib
//---------------------------------
bool JCalibrationFile::JCalibrationFile::PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string, string> &svals, string comment)
{
	// Open the item file creating the directory path if needed and
	// writing a header to it.
	ofstream *fptr = CreateItemFile(namepath, run_min, run_max, author, comment);
	if(!fptr)return true;
	
	// Loop over values
	map<string, string>::iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); iter++){
		(*fptr)<<iter->first<<"  "<<iter->second<<endl;
	}
	
	// Close file
	fptr->close();
	delete fptr;

	return true;
}

//---------------------------------
// PutCalib
//---------------------------------
bool JCalibrationFile::JCalibrationFile::PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string, string> > &svals, string comment)
{
	// We need at least one element to make this worthwhile
	if(svals.size()<1)return true;

	// Open the item file creating the directory path if needed and
	// writing a header to it.
	ofstream *fptr = CreateItemFile(namepath, run_min, run_max, author, comment);
	if(!fptr)return true;
	
	// Write list of column names from first element
	vector<string> colnames;
	map<string,string>::const_iterator iter = svals[0].begin();
	for(; iter!=svals[0].end(); iter++){
		colnames.push_back(iter->first);
	}
	
	// Write column names to header
	(*fptr)<<"#% ";
	for(unsigned int i=0; i<colnames.size(); i++){
		(*fptr)<<" "<<colnames[i];
	}
	(*fptr)<<endl;
	
	// Loop over values
	for(unsigned int j=0; j<svals.size(); j++){
		map<string, string> &mvals = svals[j];
		for(unsigned int i=0; i<colnames.size(); i++){
			(*fptr)<<mvals[colnames[i]]<<"  ";
		}
		(*fptr)<<endl;
	}

	// Close file
	fptr->close();
	delete fptr;

	return true;
}

//---------------------------------
// CreateItemFile
//---------------------------------
ofstream* JCalibrationFile::CreateItemFile(string namepath, int32_t run_min, int32_t run_max, string &author, string &comment)
{
	// Make sure the directory path exists for this namepath
	MakeDirectoryPath(namepath);

	// Open file, overwriting any existing file
	string fname = basedir + namepath;
	ofstream *fptr = new ofstream(fname.c_str());
	ofstream &f = *fptr;
	if(!f.is_open()){
		_DBG_<<"Unable to open \""<<fname<<"\"!"<<endl;
		delete fptr;
		return NULL;
	}
	
	// The comment string may come in several lines and we want each of those to be
	// prefixed with a comment character "#".
	vector<string>lines;
	size_t start = 0;
	size_t end;
	while((end=comment.find('\n', start))!=string::npos){
		lines.push_back(comment.substr(start, end-start));
		start = end+1;
	}
	if(start<comment.size())lines.push_back(comment.substr(start, (comment.size())-start));
	
	// Write comments in header
	time_t now = time(NULL);
	string host = "unknown";
	const char *hostptr = getenv("HOST");
	if(hostptr)host = hostptr;
	f<<"#"<<endl;
	f<<"# Created: "<<ctime(&now);
	f<<"# Host: "<<host<<endl;
	f<<"# Author: "<<author<<endl;
	f<<"# Namepath: "<<namepath<<endl;
	f<<"# Context: "<<GetContext()<<endl;
	f<<"# Run range: "<<run_min<<" - "<<run_max<<endl;
	f<<"#"<<endl;
	f<<"# Comment:"<<endl;
	for(unsigned int i=0; i<lines.size(); i++){
		f<<"# "<<lines[i];
		if(lines[i][lines.size()-1] != '\n')f<<endl;
	}
	f<<"#"<<endl;
	
	return fptr;
}

//---------------------------------
// MakeDirectoryPath
//---------------------------------
void JCalibrationFile::MakeDirectoryPath(string namepath)
{
	/// Create all subdirectories of basedir needed to hold the
	/// item identified by the given namepath.

	// Make sure full directory structure exists. The basedir is 
	// created first then each subdirectory within that.
	vector<string>dirs;
	size_t start = 0;
	size_t end;
	while((end=namepath.find('/', start))!=string::npos){
		dirs.push_back(namepath.substr(start, end-start));
		start = end+1;
	}
	if(start<namepath.size())dirs.push_back(namepath.substr(start, (namepath.size())-start));
	
	// Last element in dirs should be item name. Check that dirs
	// has at least one element (so we can use dirs.size()-1 below)
	// and bail now if it doesn't.
	if(dirs.size()<1)return;
	
	// Loop over subdirectories, creating them as we go
	string dirname = basedir;
	mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	for(unsigned int i=0; i<dirs.size()-1; i++){
		dirname += string("/") + dirs[i];
		mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	}
}

//---------------------------------
// GetListOfNamepaths
//---------------------------------
void JCalibrationFile::GetListOfNamepaths(vector<string> &namepaths)
{
	/// Get a list of the available namepaths by traversing the
	/// directory stucture looking for files.
	/// Note that this does NOT check whether the files are of
	/// a vaild format.
	AddToNamepathList(basedir.substr(0,basedir.length()-1), namepaths); 
}

//---------------------------------
// AddToNamepathList
//---------------------------------
void JCalibrationFile::AddToNamepathList(string dirname, vector<string> &namepaths)
{
	/// Add the files from the specified directory to the list of namepaths.
	/// and then recall ourselves for any directories found.

	// Open the directory
	DIR *dir = opendir(dirname.c_str());
	if(!dir)return;

	// Loop over directory entries
	struct dirent *dp;
	while((dp=readdir(dir))!=NULL){
		string name(dp->d_name);
		if(name=="." || name==".." || name==".svn")continue; // ignore this directory and its parent
		if(name=="info.xml" || name==".DS_Store")continue;
		
		// Check if this is a directory and if so, recall to add those
		// namepaths as well.
		struct stat st;
		string fullpath = dirname+"/"+name;
		if(stat(fullpath.c_str(),&st) == 0){
			if(st.st_mode & S_IFDIR){
				// yep, this is a directory
				AddToNamepathList(dirname+"/"+name, namepaths);
				
				// go to next iteration of loop over current directory's contents
				continue;
			}
		}
		
		// If we get here then this item is not a directory. Assume it is a valid
		// namepath and add it to the list
		namepaths.push_back(fullpath.substr(basedir.length()));
	}
	closedir(dir);
}

