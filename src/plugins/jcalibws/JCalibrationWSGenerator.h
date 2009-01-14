// $Id$
//
//    File: JCalibrationWSGenerator.h
// Created: Sun Jan 11 10:12:38 EST 2009
// Creator: davidl (on Darwin Amelia.local 9.6.0 i386)
//

#ifndef _JCalibrationWSGenerator_
#define _JCalibrationWSGenerator_

#include <JANA/jerror.h>
#include <JANA/JCalibrationGenerator.h>
#include "JCalibrationWS.h"

// Place everything in JANA namespace
namespace jana
{

class JCalibrationWSGenerator: public JCalibrationGenerator{
	public:
		JCalibrationWSGenerator(){}
		virtual ~JCalibrationWSGenerator(){}
		
		const char* Description(void){return "JCalibration Web Services";} ///< Get string indicating type of calibration this handles
		double CheckOpenable(std::string url, int run, std::string context){ ///< Test probability of opening the given calibration
			if(url.find("http://")!=0)return 0.0;
			if(url.find("/jcalibws") == string::npos )return 0.0;
			return 0.99;
		} 
		JCalibration* MakeJCalibration(std::string url, int run, std::string context){return new JCalibrationWS(url,run,context);} ///< Instantiate an JCalibration object (subclass)

};

} // Close JANA namespace

#endif // _JCalibrationWSGenerator_

