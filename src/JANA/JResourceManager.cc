// $Id$
//
//    File: JResourceManager.cc
// Created: Mon Oct 15 07:36:44 EDT 2012
// Creator: davidl (on Darwin eleanor.jlab.org 12.2.0 i386)
//

#include <stdlib.h>
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

	// Derive location of resources directory on local system.
	// This can be specified in several ways, given here in
	// order of precedence:
	//
	// 1. Passed as second argument to this constructor
	// 2. Specified in JANA:RESOURCE_DIR configuration parameter
	// 3. Specified in JANA_RESOURCE_DIR environment variable
	// 4. Use HALLD_HOME environment variable + "resources"
	// 5. Create a local directory called "resources"
	//
	// Note that in nearly all instances, no second argument should
	// be passed to the constructor so that the value can be changed
	// via run time parameters.

	// 5.
	this->resource_dir = "resources";

	// 4.
	const char *HALLD_HOME = getenv("HALLD_HOME");
	if(HALLD_HOME) this->resource_dir = string(HALLD_HOME) + "/resources";

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

#ifdef HAVE_CURL
	// Initialize CURL system
	curl_global_init(CURL_GLOBAL_ALL);
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
		if(info.find("URL")==info.end()){
			string mess = string("missing or incomplete info. for resource \"")+namepath+"\"";
			throw JException(mess);
		}
		string URL = info["URL"];

		// Do we know about this resource?
		if(resources.find(URL) != resources.end()){
			// We already have an entry for this URL.
			// set the fullpath to point to the file
			fullpath = GetLocalPathToResource(resources[URL]);
		}else{
			// We don't have an entry for this URL in our
			// resources list. Get the file and add the 
			// resource to our in-memory list, and then write the
			// in-memory list out to the resources file.

			// Get file at URL
			GetResourceFromURL(URL, fullpath);

			// Add entry to resources list
			pthread_mutex_lock(&resource_manager_mutex);
			resources[URL] = namepath;
			pthread_mutex_unlock(&resource_manager_mutex);

			// Write new resource list to file
			WriteResourceInfoFile();
		}
	}

	return fullpath;
}

//---------------------------------
// GetLocalPathToResource
//---------------------------------
string JResourceManager::GetLocalPathToResource(string namepath)
{
	return resource_dir + "/" + namepath;
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

	static int message_printed=0;
	if(!message_printed){
		printf("\nFile not compiled with CURL support! This is most likely\n");
		printf("because the curl-config script was not in the PATH when\n");
		printf("this was compiled. It was most likely not in your path\n");
		printf("because the curl-devel package was not installed on your\n");
		printf("system. \n");
		printf("The curl package is only used to automatically download\n");
		printf("the data tables needed by this package. I will now attempt\n");
		printf("to get them by running curl externally via the following:\n");
		printf("\n");
		
		message_printed = 1;
	}

	string cmd = "curl " + URL + " -o " + fullpath;
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
