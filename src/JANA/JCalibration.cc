// $Id$
//
//    File: JCalibration.cc
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

#include <sys/stat.h>

#include "JCalibration.h"
#include "JStreamLog.h"
using namespace jana;

//---------------------------------
// JCalibration    (Constructor)
//---------------------------------
JCalibration::JCalibration(string url, int32_t run, string context)
{
	this->url = url;
	this->run_number = run;
	this->context = context;
	
	retrieved_event_boundaries = false;
	
	pthread_mutex_init(&accesses_mutex, NULL);
	pthread_mutex_init(&stored_mutex, NULL);
	pthread_mutex_init(&boundaries_mutex, NULL);
}

//---------------------------------
// ~JCalibration    (Destructor)
//---------------------------------
JCalibration::~JCalibration()
{
	// Here we need to delete any data being kept in the "stored" map.
	// This is difficult since the pointers are kept as void* types
	// so we can't call delete without type casting them back into the
	// appropriate pointer type. Note that everywhere but here, the
	// stored vector is accessed through a templated method so the caller
	// provides the type information.
	//
	// The best we can do here is to check the typeid name against the list
	// of containers based on primitive types (+string) to see if we
	// can match it that way. We do this by calling the TryDelete templated
	// method which will build each of the 4 container types based on
	// the primitive type used for the template specialization parameter.
	
	// Loop over stored data containers
	map<pair<string,string>, void*>::iterator iter;
	for(iter=stored.begin(); iter!=stored.end(); iter++){
	
				if(TryDelete<         double >(iter));
		else	if(TryDelete<         float  >(iter));
		else	if(TryDelete<         int    >(iter));
		else	if(TryDelete<         long   >(iter));
		else	if(TryDelete<         short  >(iter));
		else	if(TryDelete<         char   >(iter));
		else	if(TryDelete<unsigned int    >(iter));
		else	if(TryDelete<unsigned long   >(iter));
		else	if(TryDelete<unsigned short  >(iter));
		else	if(TryDelete<unsigned char   >(iter));
		else	if(TryDelete<         string >(iter));
		else{
			_DBG_<<"Unable to delete calibration constants of type: "<<iter->first.second<<std::endl;
			_DBG_<<"namepath: "<<iter->first.first<<std::endl;
			_DBG_<<std::endl;
		}
	}
}

//---------------------------------
// PutCalib
//---------------------------------
bool JCalibration::PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, map<string, string> &svals, string comment)
{
	_DBG_<<"PutCalib(string namepath, int run_min, int run_max, int event_min, int event_max, string &author, map<string, string> &svals, string &comment="") not implemented!"<<endl;
	return true;
}

//---------------------------------
// PutCalib
//---------------------------------
bool JCalibration::PutCalib(string namepath, int32_t run_min, int32_t run_max, uint64_t event_min, uint64_t event_max, string &author, vector< map<string, string> > &svals, string comment)
{
	_DBG_<<"PutCalib(string namepath, int run_min, int run_max, int event_min, int event_max, string &author, vector< map<string, string> > &svals, string &comment="") not implemented!"<<endl;
	return true;
}

//---------------------------------
// RecordRequest
//---------------------------------
void JCalibration::RecordRequest(string namepath, string type_name)
{
	/// Record a request for a set of calibration constants.
	
	// Lock mutex when accessing "accesses" container.
	pthread_mutex_lock(&accesses_mutex);
	
	map<string, vector<string> >::iterator iter = accesses.find(namepath);
	if(iter==accesses.end()){
		vector<string> types;
		types.push_back(type_name);
		accesses[namepath] = types;
	}else{
		iter->second.push_back(type_name);
	}

	pthread_mutex_unlock(&accesses_mutex);
}

//---------------------------------
// GetEventBoundaries
//---------------------------------
void JCalibration::GetEventBoundaries(vector<uint64_t> &event_boundaries)
{
	/// Copy the event boundaries (if any) for this calibration's run
	/// into the caller supplied container. The contents of the
	/// container are replaced. If there are no boundaries, then the
	/// container is cleared and returned empty.
	
	// lock mutex
	pthread_mutex_lock(&boundaries_mutex);

	// If we haven't retrieved the boundaries yet, then do so now
	if(!retrieved_event_boundaries){
		RetrieveEventBoundaries();
		retrieved_event_boundaries = true;
	}

	// unlock mutex
	pthread_mutex_unlock(&boundaries_mutex);
	
	// Copy boundaries to the caller's container
	event_boundaries = this->event_boundaries;
}

//---------------------------------
// GetVariation
//---------------------------------
string JCalibration::GetVariation(void)
{
	/// This is a special routine that looks for a string
	/// of the format "variation=XXX" in the context string
	/// and if found, returns the "XXX" part. Otherwise, it
	/// returns "default" assuming no variation was identified.
	/// This is here for convenience since the CCDB implementation
	/// will use strings of this format to specify variations.
	/// When looking for the variation, any spaces or semi-colon
	/// found in the string will be removed along with characters
	/// following it. This is to allow semi-colon or space seperated
	/// lists in the variation.
	if(context == "default") return context;
	
	size_t pos = context.find("variation=");
	if(pos != context.npos){
		string variation = context.substr(pos+string("variation=").length());
		
		// chop of ";" and everything after it
		pos = variation.find(";");
		if(pos != context.npos){
			variation = variation.substr(0, pos);
		}
		
		// chop off space and everything after it
		pos = variation.find(" ");
		if(pos != context.npos){
			variation = variation.substr(0, pos);
		}

		return variation;
	}
	
	return "default";
}

//---------------------------------
// GetContainerType
//---------------------------------
JCalibration::containerType_t JCalibration::GetContainerType(string typeid_name)
{
	containerType_t ctype = kUnknownType;

	if(ctype==kUnknownType)ctype=TrycontainerType<         double>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         float>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         int>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         long>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         short>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         char>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<unsigned int>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<unsigned long>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<unsigned short>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<unsigned char>(typeid_name);
	if(ctype==kUnknownType)ctype=TrycontainerType<         string>(typeid_name);

	return ctype;
}

//---------------------------------
// DumpCalibrationsToFiles
//---------------------------------
void JCalibration::DumpCalibrationsToFiles(string basedir)
{
	/// This method will loop through all of the namespaces in the access list
	/// and dump them into a set of files that can be read using the JCalibrationFile
	/// sub-class. This can be used, for example, to capture the specific slice
	/// of the calibration database used for the current job for use by subsequent
	/// similar jobs.
	
	// Create base directory for writing calibrations into
	mode_t mode=S_IRWXU | S_IRWXG | S_IRWXO;
	char str[256];
	sprintf(str, "calib%d/", GetRun());
	basedir += string(str);
	mkdir(basedir.c_str(), mode);

	// Make one pass through the namepaths just to get the maximum length
	unsigned int max_namepath_len=0;
	map<string, vector<string> >::iterator iter;
	for(iter=accesses.begin(); iter!=accesses.end(); iter++){
		if(iter->first.length()>max_namepath_len)max_namepath_len=iter->first.length();
	}
	
	// Print header
	cout<<endl;
	cout<<"Dumping calibrations for this job in \""<<basedir<<"\""<<endl;
	cout<<"Calibrations obtained from:"<<endl;
	cout<<"             URL: "<<GetURL()<<endl;
	cout<<"         Context: "<<GetContext()<<endl;
	cout<<"      Run Number: "<<GetRun()<<endl;
	cout<<endl;
	string header = string("namepath") + string(max_namepath_len-8+2,' ') + "data type";
	cout<<header<<endl;
	cout<<string(max_namepath_len,'_')<<"  "<<string(20,'_') <<endl;
	
	// Loop over namepaths in access list
	for(iter=accesses.begin(); iter!=accesses.end(); iter++){
		
		// Get namepath
		string namepath = iter->first;
		
		// Check that all accesses used the same data type so we can warn user if
		// different container types were used.
		vector<string> &typeid_names = iter->second;
		if(typeid_names.size()<1){
			_DBG_<<"typeid_names.size()<1 : This should never happen!!"<<endl;
			continue;
		}
		string typeid_name = typeid_names[0];
		for(unsigned int i=1; i<typeid_names.size(); i++){
			if(typeid_names[i]!=typeid_name){
				_DBG_<<"Type mismatch for namepath "<<namepath<<endl;
				_DBG_<<"type1: "<<typeid_name<<"  type2:"<<typeid_names[i]<<endl;
				_DBG_<<"type1 will be used";
			}
		}
		
		// Parse namepath into elements separated by "/"s
		string str = namepath;
		vector<string> path_elements;
		unsigned int cutAt;
		while( (cutAt = str.find("/")) != (unsigned int)str.npos ){
			if(cutAt > 0)path_elements.push_back(str.substr(0,cutAt));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)path_elements.push_back(str);
		
		// If we have no path_elements, then we've nothing to do
		if(path_elements.size()<1){
			_DBG_<<"Unable to parse namepath "<<namepath<<endl;
			continue;
		}
		
		// Create directories to hold these constants. The directories may
		// already exist, but that's OK.
		string dir = basedir;
		for(unsigned int i=0; i<path_elements.size()-1; i++){
			dir += path_elements[i]+"/";
			mkdir(dir.c_str(), mode);
		}
		
		// Print namepath and data type
		cout<<namepath<<string(max_namepath_len-namepath.length()+2,' ')<<typeid_name<<endl;
		
		// Get container type and write file for this set of constants
		switch(GetContainerType(typeid_name)){
			case kVector:
				WriteCalibFileVector(dir, path_elements[path_elements.size()-1], namepath);
				break;
			case kMap:
				WriteCalibFileMap(dir, path_elements[path_elements.size()-1], namepath);
				break;
			case kVectorVector:
				WriteCalibFileVectorVector(dir, path_elements[path_elements.size()-1], namepath);
				break;
			case kVectorMap:
				WriteCalibFileVectorMap(dir, path_elements[path_elements.size()-1], namepath);
				break;
			default:
				_DBG_<<"Unknown container type for namepath \""<<namepath<<"\" skipping..."<<endl;
		}
	}
	
	// Write out the info.xml file to record where these values came from
	string fname_info = basedir+"info.xml";
	ofstream ofs(fname_info.c_str());
	if(ofs.is_open()){
		time_t now = time(NULL);
		string datetime(ctime(&now));
		ofs<<"<jcalibration>"<<endl;
		ofs<<"	<URL>"<<GetURL()<<"</URL>"<<endl;
		ofs<<"	<Context>"<<GetContext()<<"</Context>"<<endl;
		ofs<<"	<RunNumber>"<<GetRun()<<"</RunNumber>"<<endl;
		ofs<<"	<datetime>"<<datetime.substr(0,datetime.length()-1)<<"</datetime>"<<endl;
		ofs<<"	<timestamp>"<<now<<"</timestamp>"<<endl;
		ofs<<"</jcalibration>"<<endl;
	}
	
	// Determine an appropriate URL for these files
	
	// What we want is the full path to the top-level directory of
	// the calibration files regardless whether basedir is
	// a full or relative path. 
	char fullpath[1024]="";
	DIR *dirp = opendir("./"); // this trick suggested by getcwd man page for saving/restoring cwd
	int err = chdir(basedir.c_str());         // cd into the basedir directory
	if(!err)  err = (getcwd(fullpath, 1024)==NULL); // get full path of basedir directory
	if(dirp){
		fchdir(dirfd(dirp));                        // cd back into the working directory we started in
		closedir(dirp);
	}
	if(err != 0){
		jerr<<" Error getting full pathname to directory where the calib constants were written!" << endl;
		jerr<<" It's possible you can still access them using the info below, but this is" << endl;
		jerr<<" likely a critical error!" << endl;
		if(basedir.length() >1023) basedir = "/fatal/error/its/hopeless/"; // ;)
		strcpy(fullpath, basedir.c_str());
	}

	string newurl = string("file://")+fullpath;
	
	// Print footer
	cout<<endl;
	cout<<"To access these constants with another JANA program set your"<<endl;
	cout<<"JANA_CALIB_URL environment variable as follows:"<<endl;
	cout<<endl;
	cout<<"for tcsh:"<<endl;
	cout<<"       setenv JANA_CALIB_URL \""<<newurl<<"\""<<endl;
	cout<<endl;
	cout<<"for bash:"<<endl;
	cout<<"       export JANA_CALIB_URL=\""<<newurl<<"\""<<endl;
	cout<<endl;
}

//---------------------------------
// WriteCalibFileVector
//---------------------------------
void JCalibration::WriteCalibFileVector(string dir, string fname, string namepath)
{
	// Open output file
	string fullpath = dir+fname;
	ofstream ofs(fullpath.c_str());
	if(!ofs.is_open()){_DBG_<<"Unable to open file: "<<fullpath<<" !"<<endl; return;}

	// Get constants and check for error
	map<string,string> vals;
	if(GetCalib(namepath, vals)){
		_DBG_<<"Error getting values for: "<<namepath<<" !"<<endl;
	}else{
		// Write constants to file
		map<string,string>::iterator iter;
		for(iter=vals.begin(); iter!=vals.end(); iter++){
			ofs<<iter->second<<endl;
		}
	}
	
	ofs.close();
}

//---------------------------------
// WriteCalibFileMap
//---------------------------------
void JCalibration::WriteCalibFileMap(string dir, string fname, string namepath)
{
	// Open output file
	string fullpath = dir+fname;
	ofstream ofs(fullpath.c_str());
	if(!ofs.is_open()){_DBG_<<"Unable to open file: "<<fullpath<<" !"<<endl; return;}

	// Get constants and check for error
	map<string,string> vals;
	if(GetCalib(namepath, vals)){
		_DBG_<<"Error getting values for: "<<namepath<<" !"<<endl;
	}else{
		// Write constants to file
		map<string,string>::iterator iter;
		for(iter=vals.begin(); iter!=vals.end(); iter++){
			ofs<<iter->first<<"\t"<<iter->second<<endl;
		}
	}
	
	ofs.close();
}

//---------------------------------
// WriteCalibFileVectorVector
//---------------------------------
void JCalibration::WriteCalibFileVectorVector(string dir, string fname, string namepath)
{
	// Open output file
	string fullpath = dir+fname;
	ofstream ofs(fullpath.c_str());
	if(!ofs.is_open()){_DBG_<<"Unable to open file: "<<fullpath<<" !"<<endl; return;}

	// Get constants and check for error
	vector<map<string,string> > vals;
	if(GetCalib(namepath, vals)){
		_DBG_<<"Error getting values for: "<<namepath<<" !"<<endl;
	}else{
		// Write constants to file
		
		// Loop over rows
		vector<map<string,string> >::iterator viter;
		for(viter=vals.begin(); viter!=vals.end(); viter++){

			// Loop over columns writing values
			map<string,string>::iterator iter;
			for(iter=viter->begin(); iter!=viter->end(); iter++){
				ofs<<" "<<iter->second;
			}
			ofs<<endl;
		}
	}
	
	ofs.close();
}

//---------------------------------
// WriteCalibFileVectorMap
//---------------------------------
void JCalibration::WriteCalibFileVectorMap(string dir, string fname, string namepath)
{
	// Open output file
	string fullpath = dir+fname;
	ofstream ofs(fullpath.c_str());
	if(!ofs.is_open()){_DBG_<<"Unable to open file: "<<fullpath<<" !"<<endl; return;}

	// Get constants and check for error
	vector<map<string,string> > vals;
	if(GetCalib(namepath, vals)){
		_DBG_<<"Error getting values for: "<<namepath<<" !"<<endl;
	}else{
		// Write constants to file
		
		// Loop over rows
		vector<map<string,string> >::iterator viter;
		for(viter=vals.begin(); viter!=vals.end(); viter++){
		
			// If this is the first row, write the header line
			map<string,string>::iterator iter;
			if(viter==vals.begin()){
				ofs<<"#%";
				for(iter=viter->begin(); iter!=viter->end(); iter++){
					ofs<<"  "<<iter->first;
				}
				ofs<<endl;
			}
			
			// Loop over columns writing values
			for(iter=viter->begin(); iter!=viter->end(); iter++){
				ofs<<" "<<iter->second;
			}
			ofs<<endl;
		}
	}
	
	ofs.close();
}

