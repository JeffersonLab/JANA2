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
/// 4. Use HALLD_MY environment variable + "resources"
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
/// The calibration DB shuld have an entry for the namepath that is
/// a map of key-values with the key "URL" defined to be the location
/// of the resouce file on the web.
/// By contrast, the method GetLocalPathToResource() will give the
/// full path to where the file *should* be, but it does not try
/// and retrieve the file if it does not exist.
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

             JCalibration* GetJCalibration(void){return jcalib;}

	protected:

		// Used to get URL of remote resource
		JCalibration *jcalib;

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
		void GetResourceFromURL(const string &URL, const string &fullpath);
		
		// Argument for the external curl program in case it is used
		string curl_args;

	private:

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
	GetResource(namepath);

	// Have JCalibrationFile parse the resource file
	return jcalibfile->Get(namepath, vals, event_number);
}

} // Close JANA namespace


extern jana::JResourceManager *jRESOURCES;

#endif // _JResourceManager_

