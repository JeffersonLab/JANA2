// $Id$
//
//    File: rs_info.h
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#ifndef _rs_info_
#define _rs_info_

#include <vector>
#include <map>


#include <pthread.h>

class TH1;

/// The rs_info class holds information obtained from the server(s). It serves as a storing
/// area for information received in the cMsg callback. It is done this way so that the
/// ROOT GUI thread can access it inside of rs_mainframe::DoTimer() at it's convenience.
/// ROOT is generally not happy about 2 threads accessing it's global memory.

class rs_info{
	public:
		rs_info();
		virtual ~rs_info();
		
		class hinfo_t{
			public:
				string name;
				string title;
				string type;
				string path;
		};
		
		void Lock(void){pthread_mutex_lock(&mutex);}
		void Unlock(void){pthread_mutex_unlock(&mutex);}
		
		map<string,time_t> servers; // key=cMsg subject   val=last time we got a message from them
		string selected_server;
		string selected_hist;
		
		map<string,vector<hinfo_t> > hists;	// key=cMsg subject val is vector of histogram infos
		
		TH1 *latest_hist;
		time_t latest_hist_received_time;
		
	protected:
	
	private:
		pthread_mutex_t mutex;
};

#endif // _rs_info_

