// File: jcalibws.h 
//gsoap ns service name: jcalibws 
//gsoap ns service namespace: urn:jcalibws
//gsoap ns service location: http://localhost/cgi-bin/jcalibws
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
		unsigned long N;
		bool retval;
};

// Class to hold return values for table (think vector<map<string,string> > )
class tabledata{
	public:
		std::vector<keyvals> table;
		bool retval;
};

int ns__GetKeyValue(calinfo cinfo, keyvals &result);
int ns__GetTable(calinfo cinfo, tabledata &result);
