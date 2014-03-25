// $Id$
//
//    File: JResourceManager.cc
// Created: Mon Oct 15 07:36:44 EDT 2012
// Creator: davidl (on Darwin eleanor.jlab.org 12.2.0 i386)
//

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

#include <fstream>
using namespace std;

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif // HAVE_CURL

#include "JParameterManager.h"
#include "JResourceManager.h"
using namespace jana;

JResourceManager *jRESOURCES=NULL; // global pointer set to last instantiation

static pthread_mutex_t resource_manager_mutex = PTHREAD_MUTEX_INITIALIZER;
static string CURRENT_OUTPUT_FNAME = "";

static int mkpath(string s, mode_t mode=S_IRWXU | S_IRWXG | S_IRWXO);
static int mycurl_printprogress(void *clientp, double dltotal, double dlnow, double ultotal,  double ulnow);


//---------------------------------
// JResourceManager    (Constructor)
//---------------------------------
JResourceManager::JResourceManager(JCalibration *jcalib, string resource_dir)
{
	/// Creates a new resource manager. See class description for details

	// Record JCalibration object used to get URLs of resources
	// from calibration DB.
	this->jcalib = jcalib;
	
	// Get list of existing namepaths so we can check if they exist
	// without JCalibration subclass printing errors.
	jcalib->GetListOfNamepaths(calib_namepaths);

	// Derive location of resources directory on local system.
	// This can be specified in several ways, given here in
	// order of precedence:
	//
	// 1. Passed as second argument to this constructor
	// 2. Specified in JANA:RESOURCE_DIR configuration parameter
	// 3. Specified in JANA_RESOURCE_DIR environment variable
	// 4. Create a user directory in /tmp called "resources"
	//
	// Note that in nearly all instances, no second argument should
	// be passed to the constructor so that the value can be changed
	// via run time parameters.

	// 4.
	string user = (getenv("USER")==NULL ? "jana":getenv("USER"));
	this->resource_dir = "/tmp/" + user + "/resources";

	// 3.
	const char *JANA_RESOURCE_DIR = getenv("JANA_RESOURCE_DIR");
	if(JANA_RESOURCE_DIR) this->resource_dir = JANA_RESOURCE_DIR;

	// 2.
	if(gPARMS)gPARMS->SetDefaultParameter("JANA:RESOURCE_DIR", this->resource_dir);

	// 1.
	if(resource_dir != "") this->resource_dir = resource_dir;

	// Create a JCalibrationFile file object that uses the
	// resource directory
	string local_url = "file://" + this->resource_dir;
	jcalibfile = new JCalibrationFile(local_url, 1);

	// Try and open the resources file and read it in
	ReadResourceInfoFile();
	
	// Check if user has specified a URL base that will be used to
	// override any found in the calib DB
	overide_URL_base = false;
	if(gPARMS){
		if(gPARMS->Exists("JANA:RESOURCE_URL")){
			overide_URL_base = true;
			URL_base = "";
			gPARMS->SetDefaultParameter("JANA:RESOURCE_URL", URL_base, "Base URL to use for retrieving resources. If set, this will override any URL_base values found in the calib DB.");
		}
	}

#ifdef HAVE_CURL
	// Initialize CURL system
	curl_global_init(CURL_GLOBAL_ALL);
	curl_args = "";
#else
	// curl_args is only used of the CURL library is not
	curl_args = "--insecure";
	if(gPARMS)gPARMS->SetDefaultParameter("JANA:CURL_ARGS", curl_args, "additional arguments to be passed to the external curl program");
#endif // HAVE_CURL
	

	jRESOURCES = this;
}

//---------------------------------
// ~JResourceManager    (Destructor)
//---------------------------------
JResourceManager::~JResourceManager()
{
	if(jcalibfile) delete jcalibfile;

#ifdef HAVE_CURL
	// Cleanup CURL system
	curl_global_cleanup();
#endif // HAVE_CURL
}

//---------------------------------
// GetResource
//---------------------------------
string JResourceManager::GetResource(string namepath)
{
	string fullpath = GetLocalPathToResource(namepath);

	// If a calibration object was specified, then use it to check
	// for the URL and path to use.
	if(jcalib){
		// Get URL and local filename of resource relative to the
		// resources directory from the JCalibration object.
		map<string,string> info;
		jcalib->Get(namepath, info);
		
		// We provide 2 options here:
		//
		// Option 1.) The DB provides a "URL_base" string and a "path"
		// string. These are combined to make the full URL, and the
		// "path" is appended to the resource_dir to generate the local
		// path. One may also specify a JANA:RESOURCE_URL configuration
		// parameter to override the URL_base found in the calibDB. If
		// the JANA:RESOURCE_URL config. param. is present then the
		// URL_base parameter in the calib DB is not required.
		//
		// Option 2.) The DB provides a "URL" string only. This is used
		// as the full URL and as a key to the resources map to find
		// the relative path. If none exists, this relative path is taken
		// to be the namepath specified.
		//
		// Option 1. takes precedent. If either the "URL_base" or "path"
		// strings are present, then the other must be as well or an
		// exception is thrown. Note that "URL_base" may be specified via
		// configuration parameter in which case it need not be in the
		// calib DB. If neither "URL_base" nor "path" is present, then the
		// URL string is checked and used. If it also does not exist, an
		// exception is thrown.
		//
		// Once a URL and local path+filename have been determined, the
		// local resources map is checked to see if the URL already exists
		// here, possibly associated with another filename. If so, the
		// other path+filename is used.

		bool has_URL_base = info.find("URL_base")!=info.end();
		bool has_path = info.find("path")!=info.end();
		bool has_URL = info.find("URL")!=info.end();
		
		if(overide_URL_base) has_URL_base = true;
		
		string URL = "";
		string path = namepath;
		
		// Option 1
		if( has_URL_base || has_path ){
			if(!has_URL_base){
				_DBG_<<"URL_base=\""<<info["URL_base"]<<"\" path=\""<<info["path"]<<"\""<<endl;
				string mess = string("\"path\" specified for resource \"")+namepath+"\" but not \"URL_base\"!";
				throw JException(mess);
			}
			if(!has_path){
				_DBG_<<"URL_base=\""<<info["URL_base"]<<"\" path=\""<<info["path"]<<"\""<<endl;
				string mess = string("\"URL_base\" specified for resource \"")+namepath+"\" but not \"path\"!";
				throw JException(mess);
			}
			
			string my_URL_base = info["URL_base"];
			if(overide_URL_base) my_URL_base = URL_base;
			if(my_URL_base.length()==0){
				my_URL_base += "/";
			}else{
				if(my_URL_base[my_URL_base.length()-1] != '/') my_URL_base += "/";
			}

			path = info["path"];
			if(path.length()>0)
				if(path[0] == '/') path.erase(0,1);

			URL = my_URL_base + path;


		// Option 2
		}else if(has_URL){
		
			URL = info["URL"];


		// Insufficient info for either option
		}else{
			string mess = string("Neither \"URL_base\",\"path\" pair nor \"URL\" exist in DB for resource \"")+namepath+"\" !";
			throw JException(mess);
		}

		// Do we already have this resource?
		// (possibly a different namepath uses the same URL)
		pthread_mutex_lock(&resource_manager_mutex);
		if(resources.find(URL) != resources.end()){
			fullpath = resource_dir + "/" + resources[URL];
		}
		pthread_mutex_unlock(&resource_manager_mutex);

		// Check if resource file exists.
		bool file_exists = false;
		ifstream ifs(fullpath.c_str());
		if(ifs.is_open()){
			file_exists = true;
			ifs.close();
		}

		// Flag to decide if we need to rewrite the info file later
		bool rewrite_info_file = false;
		
		// If file doesn't exist, then download it
		if(!file_exists){
			GetResourceFromURL(URL, fullpath);

			pthread_mutex_lock(&resource_manager_mutex);
			resources[URL] = path;
			pthread_mutex_unlock(&resource_manager_mutex);

			rewrite_info_file = true;
		}else{
			// If file does exist, but URL is different, we'll need to download it,
			// replacing the existing file. The resources map will then need to be updated.
			bool redownload_required = false;
			string other_URL = "";
			map<string,string>::iterator iter;
			pthread_mutex_lock(&resource_manager_mutex);
			for(iter=resources.begin(); iter!=resources.end(); iter++){
				if(iter->second == path){
					if(iter->first != URL){
						other_URL = iter->first;
						redownload_required = true;
						break;
					}
				}
			}
			pthread_mutex_unlock(&resource_manager_mutex);
			
			if(redownload_required){
				// We must redownload the file, replacing the existing one.
				// Remove old file first and warn the user this is happening.
				jout << " Resource \"" << namepath << "\" already exists, but is" << endl;
				jout << " associated with the URL: " << other_URL << endl;
				jout << " Deleting existing file and downloading new version" << endl;
				jout << " from: " << URL << endl;
				unlink(fullpath.c_str());
				
				GetResourceFromURL(URL, fullpath);

				pthread_mutex_lock(&resource_manager_mutex);
				resources[URL] = path;
			
				// We need to remove any other entries from the resources
				// map that refer to this file using a different URL since the file
				// may have changed. 
				for(iter=resources.begin(); iter!=resources.end(); /* don't increment here */){
					map<string,string>::iterator tmp = iter; // remember iterator since we might erase it
					iter++;
					if(tmp->second == path){
						if(tmp->first != URL)resources.erase(tmp);
					}
				}
				pthread_mutex_unlock(&resource_manager_mutex);
				
				rewrite_info_file = true;
			}
		}

		// Write new resource list to file
		if(rewrite_info_file) WriteResourceInfoFile();
	}

	return fullpath;
}

//---------------------------------
// GetLocalPathToResource
//---------------------------------
string JResourceManager::GetLocalPathToResource(string namepath)
{
	 string fullpath = resource_dir + "/" + namepath;
	 if(jcalib){
	 	for(unsigned int i=0; i<calib_namepaths.size(); i++){
	 		if(calib_namepaths[i] == namepath){
	 			map<string,string> info;
				jcalib->Get(namepath, info);
				
				if(info.find("path")!=info.end()){
					fullpath = resource_dir + "/" + info["path"];
				}
				break;
			}
		}
	}
	 
	 return fullpath;
}

//---------------------------------
// ReadResourceInfoFile
//---------------------------------
void JResourceManager::ReadResourceInfoFile(void)
{
	pthread_mutex_lock(&resource_manager_mutex);

	// Clear the resources container so it is empty
	// in case we don't find the file
	resources.clear();

	// Check if resources file exists
	string fname = GetLocalPathToResource("resources");
	ifstream ifs(fname.c_str());
	if(!ifs.is_open()) {
		pthread_mutex_unlock(&resource_manager_mutex);
		return; // no resources file so just return
	}
	ifs.close();

	// The resources file exists. Read it in using the
	// JCalibrationFile class to parse it
	jcalibfile->Get("resources", resources);

	pthread_mutex_unlock(&resource_manager_mutex);
}

//---------------------------------
// WriteResourceInfoFile
//---------------------------------
void JResourceManager::WriteResourceInfoFile(void)
{
	pthread_mutex_lock(&resource_manager_mutex);

	// Get full path to resources file
	string fname = GetLocalPathToResource("resources");

	// Open file for writing, discarding any existing contents
	ofstream ofs(fname.c_str(), ios_base::out | ios_base::trunc);

	// File header
	time_t t = time(NULL);
	ofs << "#"<<endl;
	ofs << "# JANA resources file  Auto-generated DO NOT EDIT"<<endl;
	ofs << "#" << endl;
	ofs << "# " << ctime(&t); // automatically adds endl
	ofs << "#" << endl;
	ofs << "#% URL  namepath" << endl;

	map<string,string>::iterator iter;
	for(iter=resources.begin(); iter!=resources.end(); iter++){
		ofs << iter->first << "\t" << iter->second << endl;
	}
	ofs << endl;

	// Close file
	ofs.close();

	pthread_mutex_unlock(&resource_manager_mutex);
}

//---------------------------------
// GetResourceFromURL
//---------------------------------
void JResourceManager::GetResourceFromURL(const string &URL, const string &fullpath)
{
	/// Download the specified file and place it in the location specified
	/// by fullpath. If unsuccessful, a JException will be thrown with
	/// an appropriate error message.

	pthread_mutex_lock(&resource_manager_mutex);

	jout << "Downloading " << URL << " ..." << endl;
	CURRENT_OUTPUT_FNAME = fullpath;
	if(fullpath.length() > 60){
		CURRENT_OUTPUT_FNAME = string("...") + fullpath.substr(fullpath.length()-60, 60);
	}

	// Create the directory path needed to hold the resource file
	char tmp[256];
	strcpy(tmp, fullpath.c_str());
	char *path_only = dirname(tmp);
	mkpath(path_only);
	
	// Create an empty info.xml file in resources directory
	// to avoid warning from JCalibrationFile
	string info_xml = resource_dir + "/info.xml";
	ofstream ofs(info_xml.c_str());
	ofs.close();

#ifdef HAVE_CURL
	// Program has CURL library available

	// Initialize curl transaction
	CURL *curl = curl_easy_init();

	// Setup the options for the download
	char error[CURL_ERROR_SIZE] = "";
	FILE *f = fopen(fullpath.c_str(), "w");
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // allow non-secure SSL connection
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, mycurl_printprogress);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
	

	// Download the file
	curl_easy_perform(curl);
	if(error[0]!=0) cout << error << endl;

	// Close CURL
	curl_easy_cleanup(curl);

	// Close the downloaded file
	cout << endl;
	fclose(f);

#else // HAVE_CURL
	// Program does NOT have CURL library available

	string cmd = "curl " + curl_args + " " + URL + " -o " + fullpath;
	cout << cmd << endl;
	system(cmd.c_str());
#endif // HAVE_CURL

	// We may want to have an option to automatically un-compress the file here
	// if it is in a compressed format. See the bottom of getwebfile.c in the
	// Hall-D source code for the hdparsim plugin for an example of how this might
	// be done.

	// unlock mutex
	pthread_mutex_unlock(&resource_manager_mutex);
}



// The following will make all neccessary sub directories 
// in order for the specified path to exist. It was
// taken from here:
// http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
// though it was not the first answer
//----------------------------
// mkpath
//----------------------------
int mkpath(string s, mode_t mode)
{
    size_t pre=0,pos;
    std::string dir;
    int mdret=0;

    if(s[s.size()-1]!='/'){
        // force trailing / so we can handle everything in loop
        s+='/';
    }

    while((pos=s.find_first_of('/',pre))!=string::npos){
        dir=s.substr(0,pos++);
        pre=pos;
        if(dir.size()==0) continue; // if leading / first time is 0 length
        if((mdret=mkdir(dir.c_str(),mode)) && errno!=EEXIST){
            return mdret;
        }
    }
    return mdret;
}

//----------------------------
// mycurl_printprogress
//----------------------------
int mycurl_printprogress(void *clientp, double dltotal, double dlnow, double ultotal,  double ulnow)
{
	unsigned long kB_downloaded = (unsigned long)(dlnow/1024.0);
	cout << "  " << kB_downloaded << "kB  " << CURRENT_OUTPUT_FNAME << "\r";
	cout.flush();

	return 0;
}
