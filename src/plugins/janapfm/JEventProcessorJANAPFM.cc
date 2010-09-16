// Author: David Lawrence  July, 2010
//
//

#include <math.h>
#include <pthread.h>

#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;

#include <JANA/JApplication.h>
#include "JEventProcessorJANAPFM.h"
#include "JFactoryGenerator_JPMUCounts.h"
using namespace jana;


// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);

	// Initialize PFM system
	int err = pfm_initialize();
	switch(err){
		case PFM_SUCCESS:
			cout<<"--->Initialized PMU"<<endl;
			break;
		case PFM_ERR_NOTSUPP:
			cout<<"--->PMU support unavailable"<<endl;
			return;
		default:
			cout<<"--->Unknown PMU response: "<<err<<endl;
			return ;
	}
	
	// Add processor and factory generator
	app->AddProcessor(new JEventProcessorJANAPFM());
	app->AddFactoryGenerator(new JFactoryGenerator_JPMUCounts());
}
} // "C"

//------------------------------------------------------------------
// init 
//------------------------------------------------------------------
jerror_t JEventProcessorJANAPFM::init(void)
{
	// Initialize our mutex
	pthread_mutex_init(&mutex, NULL);
	
	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorJANAPFM::brun(JEventLoop *loop, int runnumber)
{	
	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorJANAPFM::evnt(JEventLoop *loop, int eventnumber)
{
	vector<const JPMUCounts*> jpmucounts;
	loop->Get(jpmucounts);

	// Loop over JPMUCounts objects
	for(unsigned int i=0; i<jpmucounts.size(); i++){
		const JPMUCounts *jpmucount = jpmucounts[i];
	
		// Look for an entry for this event in pmu_counts
		pair<string,pthread_t> key(jpmucount->name, pthread_self());
		pmu_counts_t &pc = pmu_counts[key];
		pc.counts += jpmucount->counts;
		pc.time_enabled += jpmucount->time_enabled;
		pc.time_running += jpmucount->time_running;
		pc.N++;
	}

	return NOERROR;
}

//------------------------------------------------------------------
// fini
//------------------------------------------------------------------
jerror_t JEventProcessorJANAPFM::fini(void)
{

	// We want the output file to use thread numbers 0,1,2,... instead
	// of the opaque values returned by pthread_self. Make a map of
	// these here.
	map<pthread_t,int> thr_map;
	map<pair<string,pthread_t>, pmu_counts_t>::iterator iter;
	for(iter=pmu_counts.begin(); iter!=pmu_counts.end(); iter++){
		pthread_t thr = iter->first.second;
		
		map<pthread_t,int>::iterator iter = thr_map.find(thr);
		if(iter==thr_map.end()){
			thr_map[thr] = thr_map.size();
		}
	}

	// Open output file
	string ofname = "janapfm.out";
	cout<<endl;
	cout<<"Writing Perfomance Monitor info. to \""<<ofname<<"\""<<endl;
	ofstream of(ofname.c_str());

	of<<"# thread Nevents counts time_enabled  time_running  name"<<endl;

	// Write results
	for(iter=pmu_counts.begin(); iter!=pmu_counts.end(); iter++){
		pmu_counts_t &pc = iter->second;
		string name = iter->first.first;
		pthread_t thr = iter->first.second;
		of<<thr_map[thr]<<" "<<pc.N<<" "<<pc.counts<<" "<<pc.time_enabled<<" "<<pc.time_running<<" "<<name<<endl;
	}
	
	of.close();

	return NOERROR;
}
