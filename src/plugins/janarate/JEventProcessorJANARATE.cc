// Author: David Lawrence  August 8, 2007
//
//

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
	// Initialize our mutex
	pthread_mutex_init(&mutex, NULL);
	
	initialized = false;
	finalized = false;
	Ncalls = 0;
	
	rate_tree = new TTree("rate_tree","Event Processing Rates");
	rate_tree->Branch("rates", &rate, "rate/D:delta_sec:last_sec:Ncalls/i");
	rate_tree->SetMarkerStyle(20);
	
	return NOERROR;
}

//------------------------------------------------------------------
// brun
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::brun(JEventLoop *loop, int runnumber)
{
	// Record starting time (if necessary)
	pthread_mutex_lock(&mutex);
	if(!initialized){
		initialized = true;

		// Get relevant configuration parameters
		PERIOD_EVENTS = 100;
		app->GetJParameterManager()->SetDefaultParameter("RATE:PERIOD_EVENTS", PERIOD_EVENTS);
	

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
	}
	pthread_mutex_unlock(&mutex);
	
	return NOERROR;
}

//------------------------------------------------------------------
// evnt
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::evnt(JEventLoop *loop, int eventnumber)
{
	pthread_mutex_lock(&mutex);
	if(PERIOD_EVENTS!=0 && (Ncalls%PERIOD_EVENTS)==0){
		if(Ncalls>0){
			struct itimerval cur_tmr;
			getitimer(ITIMER_REAL, &cur_tmr);

			this->rate.last_sec = (double)last_tmr.it_value.tv_sec + (double)last_tmr.it_value.tv_usec/1.0E6;
			double cur_sec = (double)cur_tmr.it_value.tv_sec + (double)cur_tmr.it_value.tv_usec/1.0E6;
			this->rate.delta_sec = rate.last_sec - cur_sec;
			this->rate.rate = (double)(Ncalls-last_Ncalls)/this->rate.delta_sec;
			this->rate.Ncalls = Ncalls-last_Ncalls;
			
			rate_tree->Fill();
		}
		getitimer(ITIMER_REAL, &last_tmr);
		last_Ncalls = Ncalls;
	}
	Ncalls++;
	pthread_mutex_unlock(&mutex);

	return NOERROR;
}

//------------------------------------------------------------------
// erun
//------------------------------------------------------------------
jerror_t JEventProcessorJANARATE::erun(void)
{
	// Record ending time (if necessary)
	pthread_mutex_lock(&mutex);
	if(!finalized){
		finalized = true;

		getitimer(ITIMER_REAL, &end_tmr);	
	}
	pthread_mutex_unlock(&mutex);
	
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
	cout<<"Average rate:"<<rate<<" Hz"<<endl;

	return NOERROR;
}

