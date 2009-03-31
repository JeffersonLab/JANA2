// File: jcalibws.h 
//gsoap ns service name: jcalibws 
//gsoap ns service namespace: urn:jcalibws
//gsoap ns service location: https://halldweb1.jlab.org/cgi-bin/jcalibws
#import "stl.h"


// Class to hold information needed to obtain a specific set of values
class calinfo{
	public:
		std::string url;
		int run;
		std::string context;
		std::string namepath;
};

// Class to hold return values (think map<string, string>)
class keyvals{
	public:
		std::vector<std::string> keys;
		std::vector<std::string> vals;
		bool retval;
};

// Class to hold return values for table (think vector<map<string,string> > )
class tabledata{
	public:
		std::vector<keyvals> table;
		bool retval;
};

// Class to hold return values for for GetListOfNamepaths
class namepathdata{
	public:
		std::vector<std::string> namepaths;
};

int ns__GetKeyValue(calinfo cinfo, keyvals &result);
int ns__GetTable(calinfo cinfo, tabledata &result);
int ns__GetListOfNamepaths(calinfo cinfo, namepathdata &result);
