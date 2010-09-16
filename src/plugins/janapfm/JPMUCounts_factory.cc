// $Id$
//
//    File: JPMUCounts_factory.cc
// Created: Sat Jul 17 06:01:24 EDT 2010
// Creator: davidl (on Linux ifarmltest 2.6.34 x86_64)
//


#include <iostream>
#include <iomanip>
using namespace std;

#include <sys/types.h>
#include <unistd.h>

#include "JPMUCounts_factory.h"
using namespace jana;

//------------------
// init
//------------------
jerror_t JPMUCounts_factory::init(void)
{
	// Set the maximum number of counters in the PMU. I don't know
	// how to tell what this number actually is, but I recall reading
	// once that AMD chips have 4. We set it to 4 for now.
	MAX_COUNTERS = 4;

	// Ideally, we'd want the user to be able to specify which events
	// they are interested in. For now though, we just make a list of 
	// all available events.
	int idx = pfm_get_event_first();
	for(int i=0; i<pfm_get_nevents(); i++){
		pfm_event_info_t info;
		memset(&info, 0, sizeof(info));
		int err = pfm_get_event_info(idx, &info);
		switch(err){
			case PFM_SUCCESS:
				//cout<<"Registering event type: "<<info.name<<endl;
				event_types.push_back(info.name);
				break;
			case PFM_ERR_NOINIT:
				cout<<"PFM not initialied error!"<<endl;
				break;
			case PFM_ERR_INVAL:
				cout<<"Invalid value error!"<<endl;
				break;
			default:
				cout<<"Unknown error: "<<err<<endl;
				break;
		}
		idx = pfm_get_event_next(idx);
	}
	
	// Now try and create a perf_event_attr structure for each event type
	for(unsigned int i=0; i<event_types.size(); i++){
		pmu_event_t pmu_event;
		int err = pfm_get_perf_event_encoding(event_types[i].c_str(), PFM_PLM3, &pmu_event.attr, NULL, NULL);
		if(err!=PFM_SUCCESS){
			_DBG_<<"ERROR filling attributes for event type \""<<event_types[i]<<"\"!"<<endl;
			continue;
		}
		pmu_event.attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING;
		pmu_events[event_types[i]] = pmu_event;
	}
	
	// Here we choose not to open any PMU "event"s so that no counts are taken
	// for the first (physics) event. This is because the first event often
	// involves many initializations which will presumably have a different
	// footprint than the rest of the events that follow.

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JPMUCounts_factory::brun(jana::JEventLoop *eventLoop, int runnumber)
{
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JPMUCounts_factory::evnt(JEventLoop *loop, int eventnumber)
{
	// Loop over pmu_events and grab the event count from any open
	// events
	map<string, pmu_event_t>::iterator iter;
	for(iter=pmu_events.begin(); iter!=pmu_events.end(); iter++){
		int fd = iter->second.fd;
		if(fd < 0)continue;

		// Stop the counter
		ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

		// Read the values for the counter. Values are:
		// values[0] = raw count
		// values[1] = TIME_ENABLED
		// values[2] = TIME_RUNNING
		uint64_t values[3]={0,0,0};
		read(fd, values, sizeof(values));
		
		// Create a JPMUCounts object and copy the data into it
		JPMUCounts *c = new JPMUCounts();
		c->name = iter->first;
		c->attr = &iter->second.attr;
		c->counts = values[0];
		c->time_enabled = values[1];
		c->time_running = values[2];
		
		_data.push_back(c);
	}
		
	// Rotate which events are being counted
	RotateEvents();

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JPMUCounts_factory::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JPMUCounts_factory::fini(void)
{
	return NOERROR;
}

//------------------
// RotateEvents
//------------------
void JPMUCounts_factory::RotateEvents(void)
{
	// Go through the list of pmu_events and close any open file
	// descriptors. Keep track of the position in the list of
	// the last fd closed so that the next MAX_COUNTERS can be 
	// opened to count the next event.
	int pos = 0;
	int last_closed_pos=-1;
	map<string, pmu_event_t>::iterator iter;
	for(iter=pmu_events.begin(); iter!=pmu_events.end(); iter++, pos++){
		if(iter->second.fd >= 0){
			close(iter->second.fd);
			iter->second.fd = -1;
			
			// What we really want is the last in the block of MAX_COUNTERS
			// which could actually be near the front of the pmu_events list
			// if it "wrapped around" (i.e. went past pmu_events.size()).
			// Therefore, we only update last_closed_pos if pos is one more
			// than it.
			if(pos==(last_closed_pos+1) || last_closed_pos==-1)last_closed_pos = pos;
		}
	}

	// Now, open MAX_COUNTERS events starting at the one after
	// last_closed_pos. If MAX_COUNTERS is greater than the 
	// size of pmu_events, then only open pmu_events.size events.
	unsigned int events_to_open = MAX_COUNTERS;
	if(events_to_open>pmu_events.size())events_to_open=pmu_events.size();
	for(unsigned int i=0; i<events_to_open; i++){
		unsigned int pos = (last_closed_pos+1+i)%pmu_events.size();

		iter = pmu_events.begin();
		for(unsigned int j=0; j<pos; j++)iter++; // advance iterator to next event we want to open
		pmu_event_t &pmu_event = iter->second;
		
		pmu_event.attr.disabled = 0; // start the counter right away
		pmu_event.fd = perf_event_open(&pmu_event.attr, (pid_t)syscall(SYS_gettid), -1, -1, 0);
	}
}

