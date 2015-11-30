// $Id: JEventSink.h 1433 2005-12-22 14:39:08Z davidl $
//
//    File: JEventSink.h
// Created: Mon Dec 19 16:15:49 EST 2005
// Creator: davidl (on Linux phecda 2.6.9-11.ELsmp athlon)
//


#ifndef _JEventSink_
#define _JEventSink_

#include <pthread.h>
#include <string>
#include <typeinfo>
using std::string;

#include "jerror.h"
#include "JFactory_base.h"
#include "JEventProcessor.h"
#include "JEventLoop.h"

namespace jana{

class JEventSink:public JEventProcessor{
	public:
		JEventSink();
		virtual ~JEventSink(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSink";}
		
	protected:
		virtual jerror_t init(void){return NOERROR;}				///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		virtual jerror_t brun_sink(JEventLoop *loop, int32_t runnumber)=0;
		virtual jerror_t evnt(JEventLoop *loop, uint64_t eventnumber){return NOERROR;}	///< Called every event.
		virtual jerror_t erun(void){return NOERROR;}				///< Called everytime run number changes, provided brun has been called.
		virtual jerror_t fini(void){return NOERROR;}				///< Called after last event of last event source has been processed.
		void AddToWriteList(string name, string tag);
		void AddAllToWriteList(JEventLoop *loop);
		void RemoveFromWriteList(string name, string tag);
		void ClearWriteList(void);
		void PrintWriteList(void);

		inline void LockSink(void){pthread_mutex_lock(&sink_mutex);}
		inline void UnlockSink(void){pthread_mutex_unlock(&sink_mutex);}
		inline bool IsInWriteList(const string &name, const string &tag){
			for(unsigned int i=0; i<factories_to_write.size(); i++){
				if(factories_to_write[i].name == name){
					if(factories_to_write[i].tag == tag)return true;
				}
			}
			return false;
		}
	
	private:
		typedef struct{
			string name;
			string tag;
		}factory_name_spec_t;
		vector<factory_name_spec_t> factories_to_write;
		
		bool initialized;
		pthread_mutex_t sink_mutex;
};

} // Close JANA namespace

#endif // _JEventSink_

