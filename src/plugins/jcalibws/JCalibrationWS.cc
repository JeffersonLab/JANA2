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
// GetCalib  --  Key-Value pairs
//---------------------------------
bool JCalibrationWS::GetCalib(string namepath, map<string, string> &svals)
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
	
	// We may need to lock a mutex here to make sure only 1 thread
	// accesses the "soap" struct at a time.  1/15/2009 DL

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
// GetCalib  --  Table data
//---------------------------------
bool JCalibrationWS::GetCalib(string namepath, vector< map<string, string> > &vsvals)
{
	// Copy to private calinfo struct so we can be thread-safe
	calinfo mycinfo = cinfo;
	mycinfo.namepath=namepath;
	tabledata result;

	// Make sure return map is empty to start with
	vsvals.clear();
	
	// Make sure our "soap" pointer is at least not NULL
	if(!soap){
		_DBG_<<"JCalibration soap pointer NULL!"<<endl;
		return true;
	}
	
	// We may need to lock a mutex here to make sure only 1 thread
	// accesses the "soap" struct at a time.  1/15/2009 DL

	// Get the values from the Web Service and check the results
   if (soap_call_ns__GetTable(soap, NULL, NULL, mycinfo, result) == SOAP_OK){
	
		unsigned int Ncolumns=0;
		for(unsigned int i=0; i<result.table.size(); i++){
			
			keyvals &mykeyvals = result.table[i];
		
			// The table consists of a number of rows and every row
			// has both a key and a value for every column (the key being 
			// the column name). And yes, it is redundant to keep the column
			// name with every value but that is how it is.
			//
			// Here, we get the size of the first keys vector and use
			// that to check all other keys vectors as well as vals vectors.
			if(i==0)Ncolumns = mykeyvals.keys.size();
	
			if(mykeyvals.keys.size()!=Ncolumns || mykeyvals.vals.size()!=Ncolumns){
				_DBG_<<"keys and values vector sizes don't match!"<<endl;
				return true;
			}else{
				// Success! Copy results into svals map.
				map<string, string> svals;
				for(unsigned int i=0; i<Ncolumns; i++){
					svals[mykeyvals.keys[i]] = mykeyvals.vals[i];
				}
				vsvals.push_back(svals);
			}
		}
		return result.retval;
   }else{ // an error occurred  
      soap_print_fault(soap, stderr); // display the SOAP fault on the stderr stream
		return true;
	}

	// We should never actually get here.
	return false; // everything is OK
}

//---------------------------------
// GetListOfNamepaths
//---------------------------------
void JCalibrationWS::GetListOfNamepaths(vector<string> &namepaths)
{
	// Copy to private calinfo struct so we can be thread-safe
	calinfo mycinfo = cinfo;
	namepathdata result;

	// Make sure return map is empty to start with
	namepaths.clear();
	
	// Make sure our "soap" pointer is at least not NULL
	if(!soap){
		_DBG_<<"JCalibration soap pointer NULL!"<<endl;
		return;
	}
	
	// We may need to lock a mutex here to make sure only 1 thread
	// accesses the "soap" struct at a time.  1/15/2009 DL

	// Get the values from the Web Service and check the results
   if (soap_call_ns__GetListOfNamepaths(soap, NULL, NULL, mycinfo, result) == SOAP_OK){
		// Success! Copy results into namepaths.
		namepaths = result.namepaths;
		return;
   }
	
	// an error occurred  
	soap_print_fault(soap, stderr); // display the SOAP fault on the stderr stream
}


