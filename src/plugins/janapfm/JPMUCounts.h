// $Id$
//
//    File: JPMUCounts.h
// Created: Sat Jul 17 06:01:24 EDT 2010
// Creator: davidl (on Linux ifarmltest 2.6.34 x86_64)
//

#ifndef _JPMUCounts_
#define _JPMUCounts_

extern "C" {
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <perfmon/perf_event.h>
}

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class JPMUCounts:public jana::JObject{
	public:
		JOBJECT_PUBLIC(JPMUCounts);

		string name;
		const struct perf_event_attr *attr;
		uint64_t counts;
		uint64_t time_enabled;
		uint64_t time_running;

		void toStrings(vector<pair<string,string> > &items)const{
			AddString(items, "counts", "%ld", counts);
			AddString(items, "time_enabled", "%ld", time_enabled);
			AddString(items, "time_running", "%ld", time_running);
			AddString(items, "event", "%s", name.c_str());
		}

		// Constructor
		JPMUCounts(){
			attr = NULL;
			counts = time_enabled = time_running = 0;
		}
	
};

#endif // _JPMUCounts_

