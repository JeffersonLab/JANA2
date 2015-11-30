// $Id$
//
//    File: JResourceManager.h
// Created: Mon Oct 15 07:36:44 EDT 2012
// Creator: davidl (on Darwin eleanor.jlab.org 12.2.0 i386)
//

#include <string>
using std::string;

#include <JANA/JCalibrationFile.h>

#ifndef _JResourceManager_
#define _JResourceManager_

#include <JANA/jerror.h>

// Place everything in JANA namespace
namespace jana{


/// The JResourceManager class is used to manage local resource files.
/// These would typically be larger files that are costly to download
/// every time the program is run. (i.e. if they come from the 
/// JCalibration system.) Files are kept in a central location
/// on the local filesystem so only one copy needs to exist.
///
/// The JResourceManager constructor takes two arguments. The first
/// is a pointer to a JCalibration object. If not NULL, this is used
/// to retrieve the URL of the file's location on the web. This allows
/// the JResourceManager to automatically download the file if needed.
///
/// If no JCalibration object is supplied (i.e. it's NULL), then it will 
/// simply look for a file with the specified namepath relative to the 
/// root directory used to hold the resources. The root resource directory 
/// can be specified multiple ways. These are, in order of precedence:
///
/// 1. Passed as second argument to the constructor
/// 2. Specified in JANA:RESOURCE_DIR configuration parameter
/// 3. Specified in JANA_RESOURCE_DIR environment variable
/// 4. Specified in JANA:RESOURCE_DEFAULT_PATH configuration parameter
/// 5. Create a user directory in /tmp called "resources"
///
/// Note that in nearly all instances, no second argument should
/// be passed to the constructor so that the value can be changed
/// via run time parameters.
///
/// Resource files can be of any format but if they are in an ASCII
/// format compatible with JCalibrationFile then the file can be
/// parsed using the Get(...) methods of JResourceManager which map
/// directly to those of JCalibrationFile.
///
/// To get the location of a resource on the local file system,
/// use the method:
///
/// string GetResource(string namepath);
///
/// This will return a string with the full path to the resource file on
/// the local file system. The call will automatically download
/// the resource and install it if it does not already exist locally.
/// The download location will be retrieved using the specified
/// namepath and the JCalibration object passed in to the constructor.
/// The calibration DB should have an entry for the namepath that is
/// a map of key-values with two options for how the URL is specified:
///
/// Option 1.) The DB provides a "URL_base" string and a "path"
/// string. These are combined to make the full URL, and the
/// "path" is appended to the resource_dir to generate the local
/// path. Alternatively, "URL_base" may be provided via the
/// JANA:RESOURCE_URL configuration parameter in which case it need
/// not be present in the calib DB. If the config. parameter is
/// supplied, it will be used instead of any "URL_base" values found
/// in the calib DB.
///
/// Option 2.) The DB provides a "URL" string only. This is used
/// as the full URL and as a key to the resources map to find
/// the local path. If none exists, this local path is taken
/// to be the namepath specified (appended to resource_dir).
///
/// Option 1. takes precedent. If either the "URL_base" or "path"
/// strings are present, then the other must be as well or an
/// exception is thrown. If neither is present, then the URL
/// string is checked and used. If it also does not exist, an
/// exception is thrown.
///
///
/// A text file named "resources" is maintained at the top level of
/// the resources directory tree to record what URLs have been
/// downloaded and where the files are stored. This file is necessary
/// if option 2 above is used to store URLs in the calibration DB,
/// but is only informational if option 1 is used. It is ignored
/// completely if no calibration database is used.
///
/// The templated Get(namepath, T vals [, event_number]) method will
/// first call the GetResource() method described above, but will
/// then use a JCalibrationFile object to parse the resource file,
/// filling the container passed in for "vals". See the documentation
/// for the JCalibration class for more info on the allowed types
/// for "vals".


class JResourceManager{
	public:
                          JResourceManager(JCalibration *jcalib=NULL, string resource_dir="");
                  virtual ~JResourceManager();

    template<class T> bool Get(string namepath, T &vals, int event_number=0);

                    string GetResource(string namepath);
                    string GetLocalPathToResource(string namepath);
        map<string,string> GetLocalResources(void){return resources;}

             JCalibration* GetJCalibration(void){return jcalib;}

                      void GetResourceFromURL(const string &URL, const string &fullpath);
					  string Get_MD5(string fullpath);

	protected:

		// Used to get URL of remote resource
		JCalibration *jcalib;
		
		// Keep list of namepaths in JCalibration
		vector<string> calib_namepaths;

		// Used to convert files to values in STL containers
		JCalibrationFile *jcalibfile;

		// Full path to top-most directory of resource files
		string resource_dir;

		// Map of URLs to namepaths for existing resources
		// key is URL and value is relative path (which should
		// be the same as the namepath)
		map<string,string> resources;

		void ReadResourceInfoFile(void);
		void WriteResourceInfoFile(void);
		
		// Argument for the external curl program in case it is used
		string curl_args;

	private:
	
		// Holds user specified URL_base that superceeds any found
		// in calib DB
		bool overide_URL_base;
		string URL_base;
		bool check_md5;

};

//----------------------
// Get
//----------------------
template<class T> bool JResourceManager::Get(string namepath, T &vals, int event_number)
{
	/// Get the specified resource and parse it, placing the values in the
	/// specified "vals" container. This first calls GetResource(namepath)
	/// to download the resource (if necessary) and then uses a
	/// JCalibrationFile::Get() method to parse the file and fill the
	/// "vals" container.

	// Call to GetResource to (optionally) download and install resource file
	string fullpath = GetResource(namepath);
	string path = fullpath.substr(resource_dir.size()+1); // chop off resource_dir + "/"

	// Have JCalibrationFile parse the resource file
	return jcalibfile->Get(path, vals, event_number);
}

} // Close JANA namespace


extern jana::JResourceManager *jRESOURCES;

#endif // _JResourceManager_

