// $Id$
//
//    File: JCalibrationWS.cc
// Created: Sun Jan 11 10:11:24 EST 2009
// Creator: davidl (on Darwin Amelia.local 9.6.0 i386)
//

#include <iostream>
#include <fstream>
using namespace std;

#include <JANA/JApplication.h>
#include "JCalibrationWSGenerator.h"
#include "JCalibrationWS.h"
using namespace jana;


// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddCalibrationGenerator(new JCalibrationWSGenerator());
}
} // "C"

//---------------------------------
// JCalibrationWS    (Constructor)
//---------------------------------
JCalibrationWS::JCalibrationWS(string url, int run, string context):JCalibration(url, run, context)
{
	run_min = run_max = run_found = run;
	
	soap = soap_new();
	cinfo.url = url;
	cinfo.run = run;
	cinfo.context = context;
}

//---------------------------------
// ~JCalibrationWS    (Destructor)
//---------------------------------
JCalibrationWS::~JCalibrationWS()
{

}

//---------------------------------
// Get  --  Key-Value pairs
//---------------------------------
bool JCalibrationWS::Get(string namepath, map<string, string> &svals)
{
	// Copy to private calinfo struct so we can be thread-safe
	calinfo mycinfo = cinfo;
	mycinfo.namepath=namepath;
	keyvals result;

	// Make sure return map is empty to start with
	svals.clear();
	
	// Make sure our "soap" pointer is at least not NULL
	if(!soap){
		_DBG_<<"JCalibration soap pointer NULL!"<<endl;
		return true;
	}

	// Get the values from the Web Service and check the results
   if (soap_call_ns__GetKeyValue(soap, NULL, NULL, mycinfo, result) == SOAP_OK){
		if(result.keys.size()!=result.vals.size()){
			_DBG_<<"keys and values vector sizes don't match!"<<endl;
			return true;
		}else{
			// Success! Copy results into svals map.
			for(unsigned int i=0; i<result.keys.size(); i++){
				svals[result.keys[i]] = result.vals[i];
			}
			return result.retval;
		}
   }else{ // an error occurred  
      soap_print_fault(soap, stderr); // display the SOAP fault on the stderr stream
		return true;
	}

	// We should never actually get here.
	return false; // everything is OK
}

//---------------------------------
// Get  --  Table data
//---------------------------------
bool JCalibrationWS::Get(string namepath, vector< map<string, string> > &svals)
{

	return false; // everything is OK
}
