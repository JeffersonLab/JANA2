// Author: David Lawrence  August 8, 2007
//
//

#include <sys/time.h>
#include <sys/resource.h>
		 
#include <iostream>
#include <fstream>
using namespace std;

#include <TTree.h>

#include <JANA/JApplication.h>
#include "JEventProcessorJANARATE.h"
using namespace jana;


// Routine used to allow us to register our JEventSourceGenerator
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new JEventProcessorJANARATE());
}
} // "C"

//------------------------------------------------------------------
// init 
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::init(void)
{
//	// Initialize our mutex
//	pthread_mutex_init(&mutex, NULL);
	
//	initialized = false;
	finalized = false;
	Ncalls = 0;
	prescale = 100;
	
	gPARMS->SetDefaultParameter("RATE:PRESCALE", prescale, "Prescale entries in rate tree by this");
	
	app->RootReadLock();
	rate_tree = new TTree("rate_tree","Event Processing Rates");
	rate_tree->Branch("rates", &rate, "tot_rate/D:tot_integrated_rate:thread_rate:thread_delta_sec:cpu:mem_MB:threadid/i");
	rate_tree->SetMarkerStyle(20);
	app->RootUnLock();
	
	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::brun(JEventLoop *loop, int32_t runnumber)
{
	// Record starting time (if necessary)
//	pthread_mutex_lock(&mutex);
//	if(!initialized){
//		initialized = true;

//		// Get relevant configuration parameters
//		PERIOD_EVENTS = 100;
//		app->GetJParameterManager()->SetDefaultParameter("RATE:PERIOD_EVENTS", PERIOD_EVENTS);
	
		// Get starting time. If hi-res timer is not set, then set it
		// (n.b. JANA framework should always set it, but this legacy
		// code shoudln't hurt anything.)
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
//	}
//	pthread_mutex_unlock(&mutex);
	
	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::evnt(JEventLoop *loop, uint64_t eventnumber)
{
	double loadavg = 0.0;

	if(eventnumber%prescale != 0) return NOERROR;

	// Get system resource usage
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
//	double t_user = (double)usage.ru_utime.tv_sec + 1.0E-6*(double)usage.ru_utime.tv_usec;
//	double t_sys  = (double)usage.ru_stime.tv_sec + 1.0E-6*(double)usage.ru_stime.tv_usec;
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
	myrate.tot_rate = app->GetRate();
	myrate.tot_integrated_rate = app->GetIntegratedRate();
	myrate.thread_rate = loop->GetInstantaneousRate(); // only updated every 2 seconds!
	myrate.thread_delta_sec = loop->GetLastEventProcessingTime(); // updated every event
	myrate.threadid = (unsigned int)(0xFFFFFFFF & (unsigned long)loop->GetPThreadID());
	myrate.cpu = loadavg;
	myrate.mem_MB = mem_usage;
	
	app->RootWriteLock();
	this->rate = myrate;
	rate_tree->Fill();
	Ncalls++; // do this in ROOT mutex lock just to make sure one thread updates it at a time
	app->RootUnLock();

//	pthread_mutex_lock(&mutex);
//	if(PERIOD_EVENTS!=0 && (Ncalls%PERIOD_EVENTS)==0){
//		if(Ncalls>0){
//			struct itimerval cur_tmr;
//			getitimer(ITIMER_REAL, &cur_tmr);
//
//			this->rate.last_sec = (double)last_tmr.it_value.tv_sec + (double)last_tmr.it_value.tv_usec/1.0E6;
//			double cur_sec = (double)cur_tmr.it_value.tv_sec + (double)cur_tmr.it_value.tv_usec/1.0E6;
//			this->rate.delta_sec = rate.last_sec - cur_sec;
//			this->rate.rate = (double)(Ncalls-last_Ncalls)/this->rate.delta_sec;
//			this->rate.Ncalls = Ncalls-last_Ncalls;
//			
//			rate_tree->Fill();
//		}
//		getitimer(ITIMER_REAL, &last_tmr);
//		last_Ncalls = Ncalls;
//	}
//	Ncalls++;
//	pthread_mutex_unlock(&mutex);

	return NOERROR;
}

//------------------------------------------------------------------
// erun
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::erun(void)
{
	// Record ending time (if necessary)
//	pthread_mutex_lock(&mutex);
	if(!finalized){
		finalized = true;

		getitimer(ITIMER_REAL, &end_tmr);	
	}
//	pthread_mutex_unlock(&mutex);
	
	return NOERROR;
}

//------------------------------------------------------------------
// fini
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::fini(void)
{
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
	cout<<"Avg. rate (JApplication):"<<app->GetIntegratedRate()<<" Hz"<<endl;

	return NOERROR;
}

