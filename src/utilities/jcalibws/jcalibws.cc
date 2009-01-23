
#include "soapH.h"
#include "jcalibws.nsmap"

#include <JANA/JCalibrationFile.h>
using namespace jana;

string LOCAL_URL = "file:///group/halld/calib";

//----------------
// main
//----------------
int main(int narg, char *argv[]) 
{ 
   // create soap context and serve one CGI-based request: 
   soap_serve(soap_new());
	
	return 0;
}

//----------------
// ns__GetKeyValue
//----------------
int ns__GetKeyValue(struct soap *soap, calinfo cinfo, keyvals &result)
{
	// Create a temporary JCalibration Object
	JCalibrationFile jcalib(LOCAL_URL, cinfo.run, cinfo.context);
	
	// Get the requested values
	map<string, string> svals;
	result.retval = jcalib.Get(cinfo.namepath, svals);
	
	// Copy results into result structure
	map<string, string>::iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); iter++){
		result.keys.push_back(iter->first);
		result.vals.push_back(iter->second);
	}

   return SOAP_OK; 
}

//----------------
// ns__GetTable
//----------------
int ns__GetTable(struct soap *soap, calinfo cinfo, tabledata &result)
{
	// Create a temporary JCalibration Object
	JCalibrationFile jcalib(LOCAL_URL, cinfo.run, cinfo.context);
	
	// Get the requested values
	vector<map<string, string> > vsvals;
	result.retval = jcalib.Get(cinfo.namepath, vsvals);
	
	// Copy results into result structure
	map<string, string>::iterator iter;
	for(unsigned int i=0; i<vsvals.size(); i++){
		map<string, string> &svals=vsvals[i];
		keyvals mykeyvals;
		for(iter=svals.begin(); iter!=svals.end(); iter++){
			mykeyvals.keys.push_back(iter->first);
			mykeyvals.vals.push_back(iter->second);
		}
		result.table.push_back(mykeyvals);
	}

   return SOAP_OK; 
}

//----------------
// ns__GetListOfNamepaths
//----------------
int ns__GetListOfNamepaths(struct soap *soap, calinfo cinfo, namepathdata &result)
{
	// Create a temporary JCalibration Object
	JCalibrationFile jcalib(LOCAL_URL, cinfo.run, cinfo.context);
	
	// Get the requested values
	vector<string> namepaths;
	jcalib.GetListOfNamepaths(result.namepaths);

   return SOAP_OK; 
}
