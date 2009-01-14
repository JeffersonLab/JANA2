
#include "soapH.h"
#include "jcalibws.nsmap"

#include <JANA/JCalibrationFile.h>
using namespace jana;

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
	JCalibrationFile jcalib("file:///Users/davidl/HallD/calib", cinfo.run, cinfo.context);
	
	// Get the requested values
	map<string, string> svals;
	result.retval = jcalib.Get(cinfo.namepath, svals);
	
	// Copy results into result structure
	map<string, string>::iterator iter;
	for(iter=svals.begin(); iter!=svals.end(); iter++){
		result.keys.push_back(iter->first);
		result.vals.push_back(iter->second);
	}
	result.N = result.keys.size();

   return SOAP_OK; 
}

//----------------
// ns__GetTable
//----------------
int ns__GetTable(struct soap *soap, calinfo cinfo, tabledata &result)
{

   return SOAP_OK; 
}
