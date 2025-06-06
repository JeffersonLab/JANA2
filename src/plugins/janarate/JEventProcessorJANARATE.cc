
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <sys/time.h>
#include <sys/resource.h>
		 
#include <iostream>
#include <fstream>
#include <mutex>
using namespace std;

#include <TTree.h>
#include <TFile.h>

#include "JEventProcessorJANARATE.h"

TFile *rootfile = nullptr;
mutex mtxroot;

// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->Add(new JEventProcessorJANARATE());
}
} // "C"

//------------------------------------------------------------------
// Init
//------------------------------------------------------------------
void JEventProcessorJANARATE::Init(void)
{
//	// Initialize our mutex
//	pthread_mutex_init(&mutex, NULL);
	
//	initialized = false;
	finalized = false;
	Ncalls = 0;
	prescale = 100;

	GetApplication()->GetJParameterManager()->SetDefaultParameter(
		"RATE:PRESCALE", 
		prescale, 
		"Prescale entries in rate tree by this");
	
	getitimer(ITIMER_REAL, &start_tmr);
	if(start_tmr.it_value.tv_sec==0 && start_tmr.it_value.tv_usec==0){
		struct itimerval value, ovalue;
		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = 0;
		value.it_value.tv_sec = 1000000;
		value.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &value, &ovalue);

		getitimer(ITIMER_REAL, &start_tmr);
	}

	lock_guard<std::mutex> lck(mtxroot);
	rootfile = new TFile("janarate.root", "RECREATE");
	rate_tree = new TTree("rate_tree","Event Processing Rates");
	rate_tree->Branch("rates", &rate, "tot_rate/D:tot_integrated_rate:thread_rate:thread_delta_sec:cpu:mem_MB:threadid/i");
	rate_tree->SetMarkerStyle(20);

}

//------------------------------------------------------------------
// Process
//------------------------------------------------------------------
void JEventProcessorJANARATE::Process(const std::shared_ptr<const JEvent>& aEvent)
{
	double loadavg = 0.0;

	if(aEvent->GetEventNumber()%prescale != 0) return;

	// Get system resource usage
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double mem_usage = (double)(usage.ru_maxrss)/1024.0; // convert to MB
	
#ifdef __linux__
	// Get CPU utilization
	static uint64_t last_sum3=0.0, last_sum4=0.0;
	FILE *fp = fopen("/proc/stat","r");
	if(fp){
		uint64_t a[4] = {0,0,0,0};
		fscanf(fp, "%*s %ld %ld %ld %ld", &a[0], &a[1], &a[2], &a[3]);
		fclose(fp);

		uint64_t sum3 = a[0] + a[1] + a[2];
		uint64_t sum4 = sum3 + a[3];
		if(last_sum4!=0) loadavg = ((double)(sum3 - last_sum3))/((double)(sum4 - last_sum4));
		last_sum3 = sum3;
		last_sum4 = sum4;
	}
#endif // __linux__

#ifdef __APPLE__
	// Getting CPU utilization on Mac OS X doesn't seem trivial from
	// a quick web search so we don't implement it here yet.
#endif // __APPLE__

	// Fill local variable outside mutex lock in case
	rate_t myrate;
	auto app = GetApplication();
	myrate.tot_rate = app->GetInstantaneousRate();
	myrate.tot_integrated_rate = app->GetIntegratedRate();
//	myrate.thread_rate = loop->GetInstantaneousRate(); // only updated every 2 seconds!
//	myrate.thread_delta_sec = loop->GetLastEventProcessingTime(); // updated every event
//	myrate.threadid = (unsigned int)(0xFFFFFFFF & (unsigned int)&(std::this_thread::get_id()));
	myrate.cpu = loadavg;
	myrate.mem_MB = mem_usage;
	
	lock_guard<std::mutex> lck(mtxroot);
	this->rate = myrate;
	rate_tree->Fill();
	Ncalls++; // do this in ROOT mutex lock just to make sure one thread updates it at a time
}

//------------------------------------------------------------------
// Finish
//------------------------------------------------------------------
void JEventProcessorJANARATE::Finish(void)
{
	// Record ending time (if necessary)
	if(!finalized){
		finalized = true;

		getitimer(ITIMER_REAL, &end_tmr);	
	}

	cout<<"Start time:"<<endl;
	cout<<"    sec="<<start_tmr.it_value.tv_sec<<endl;
	cout<<"   usec="<<start_tmr.it_value.tv_usec<<endl;
	cout<<endl;
	cout<<"End time:"<<endl;
	cout<<"    sec="<<end_tmr.it_value.tv_sec<<endl;
	cout<<"   usec="<<end_tmr.it_value.tv_usec<<endl;
	cout<<endl;

	double start_sec = (double)start_tmr.it_value.tv_sec + (double)start_tmr.it_value.tv_usec/1.0E6;
	double end_sec = (double)end_tmr.it_value.tv_sec + (double)end_tmr.it_value.tv_usec/1.0E6;
	double delta_sec = start_sec - end_sec;
	double rate = (double)Ncalls/delta_sec;
	
	cout<<"Elapsed time:"<<delta_sec<<" sec"<<endl;
	cout<<"Total events:"<<Ncalls<<endl;
	cout<<"Avg. rate (janarate):"<<rate<<" Hz"<<endl;
	cout<<"Avg. rate (JApplication):"<<GetApplication()->GetIntegratedRate()<<" Hz"<<endl;

	rootfile->Write();
	delete rootfile;
}

