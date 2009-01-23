// $Id$
//
//    File: JCalibrationWS.h
// Created: Sun Jan 11 10:11:24 EST 2009
// Creator: davidl (on Darwin Amelia.local 9.6.0 i386)
//

#ifndef _JCalibrationWS_
#define _JCalibrationWS_

#include <JANA/jerror.h>
#include <JANA/JCalibration.h>

#include "soapH.h" // obtain the generated stub  
#include "jcalibws.nsmap" // obtain the generated XML namespace mapping table for the Quote service 

// Place everything in JANA namespace
namespace jana
{

class JCalibrationWS:public JCalibration{
	public:
		JCalibrationWS(string url, int run, string context="default");
		virtual ~JCalibrationWS();
		const char* className(void){return "JCalibrationWS";}
		
		bool GetCalib(string namepath, map<string, string> &svals);
		bool GetCalib(string namepath, vector< map<string, string> > &vsvals);
		void GetListOfNamepaths(vector<string> &namepaths);

	private:
		JCalibrationWS(void); // prevent use of default constructor
		
		struct soap *soap;
		calinfo cinfo;
};

} // Close JANA namespace

#endif // _JCalibrationWS_

