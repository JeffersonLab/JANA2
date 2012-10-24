// $Id$
//
//    File: JCalibrationFile.h
// Created: Fri Jul  6 19:45:56 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#ifndef _JCalibrationFile_
#define _JCalibrationFile_

#include <fstream>

#include "jerror.h"
#include "JCalibration.h"

// Place everything in JANA namespace
namespace jana{

class JCalibrationFile:public JCalibration{
	public:
		JCalibrationFile(string url, int run, string context="default");
		virtual ~JCalibrationFile();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JCalibrationFile";}

		bool GetCalib(string namepath, map<string, string> &svals, int event_number=0);
		bool GetCalib(string namepath, vector< map<string, string> > &svals, int event_number=0);
		bool PutCalib(string namepath, int run_min, int run_max, int event_min, int event_max, string &author, map<string, string> &svals, string comment="");
		bool PutCalib(string namepath, int run_min, int run_max, int event_min, int event_max, string &author, vector< map<string, string> > &svals, string comment="");
		void GetListOfNamepaths(vector<string> &namepaths);
		
	protected:
		
		std::ofstream* CreateItemFile(string namepath, int run_min, int run_max, string &author, string &comment);
		void MakeDirectoryPath(string namepath);
	
	private:
		JCalibrationFile();
		
		string basedir;

		void AddToNamepathList(string dir, vector<string> &namepaths);
};

} // Close JANA namespace


#endif // _JCalibrationFile_

