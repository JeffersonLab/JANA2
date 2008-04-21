// $Id$
//
//    File: JCalibrationFile.h
// Created: Fri Jul  6 19:45:56 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#ifndef _JCalibrationFile_
#define _JCalibrationFile_

#include "jerror.h"
#include "JCalibration.h"

// Place everything in JANA namespace
namespace jana
{

class JCalibrationFile:public JCalibration{
	public:
		JCalibrationFile(string url, int run, string context="default");
		virtual ~JCalibrationFile();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JCalibrationFile";}
		
		bool Get(string namepath, map<string, string> &svals);
		bool Get(string namepath, vector< map<string, string> > &svals);
		
	protected:
	
	
	private:
		JCalibrationFile();
		
		string basedir;

};

} // Close JANA namespace


#endif // _JCalibrationFile_

