// $Id$
//
//    File: JEventProcessor_janaview.h
// Created: Fri Oct  3 08:14:14 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JEventProcessor_janaview_
#define _JEventProcessor_janaview_

#include <set>
using std::set;

#include <TLatex.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include "jv_mainframe.h"

#include <JVFactoryInfo.h>

class JEventProcessor_janaview:public jana::JEventProcessor{
	public:
	
		class CGobj{
			public:
				CGobj(string nametag):nametag(nametag),x1(0.0),y1(0.0),x2(0.0),y2(0.0),rank(0){
					TLatex ltmp(0.0, 0.0, nametag.c_str());
					ltmp.SetTextSizePixels(20);
					ltmp.GetBoundingBox(w, h);
				}
			
				string nametag;
				Float_t x1;
				Float_t y1;
				Float_t x2;
				Float_t y2;
				Float_t ymid;
				UInt_t w;   // width in pixels
				UInt_t h;   // height in pixels
				Int_t rank; // rank
				
				set<CGobj*> callees;
		};
		
		class CGrankprop{
			public:
				CGrankprop(void):totwidth(0),totheight(0){}
				vector<CGobj*> cgobjs;
				Int_t totwidth;  // pixels -- just max box size, no spacing
				Int_t totheight; // pixels -- sum of box heights, no spacing
		};
	
	
		JEventProcessor_janaview();
		~JEventProcessor_janaview();
		const char* className(void){return "JEventProcessor_janaview";}
		
		pthread_mutex_t mutex;
		pthread_cond_t cond;
		JEventLoop *loop;
		int eventnumber;

		void Lock(void){pthread_mutex_lock(&mutex);}
		void Unlock(void){pthread_mutex_unlock(&mutex);}
		
		void NextEvent(void);
		string MakeNametag(const string &name, const string &tag);
		void GetObjectTypes(vector<JVFactoryInfo> &facinfo);
		void GetAssociatedTo(JObject *jobj, vector<const JObject*> &associatedTo);
		void MakeCallGraph(string nametag="");

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

extern JEventProcessor_janaview *JEP;

#endif // _JEventProcessor_janaview_

