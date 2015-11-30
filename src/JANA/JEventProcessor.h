// $Id: JEventProcessor.h 1709 2006-04-26 20:34:03Z davidl $
//
//    File: JEventProcessor.h
// Created: Wed Jun  8 12:31:12 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JEventProcessor_
#define _JEventProcessor_

#include <pthread.h>
#include <vector>
using std::vector;

#include "jerror.h"
#include "JParameterManager.h"
#include "JObject.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"

// Place everything in JANA namespace
namespace jana{

class JEventLoop;
class JApplication;

class JEventProcessor{
	public:
		JEventProcessor();
		virtual ~JEventProcessor();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventProcessor";}
		
		virtual jerror_t init(void);						                        ///< Called once at program start.
		virtual jerror_t brun(JEventLoop *eventLoop, int32_t runnumber);	   ///< Called everytime a new run number is detected.
//		virtual jerror_t evnt(JEventLoop *eventLoop, int eventnumber);	      ///< DEPRECATED. Use the uint64_t version instead
		virtual jerror_t evnt(JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		virtual jerror_t erun(void);						                        ///< Called everytime run number changes, provided brun has been called.
		virtual jerror_t fini(void);						                        ///< Called after last event of last event source has been processed.
		
		inline int init_was_called(void){return init_called;}
		inline int brun_was_called(void){return brun_called;}
		inline int evnt_was_called(void){return evnt_called;}
		inline int erun_was_called(void){return erun_called;}
		inline int fini_was_called(void){return fini_called;}
		inline int GetBRUN_RunNumber(void){return brun_runnumber;}
		
		inline void Clear_init_called(void){init_called=0;}
		inline void Clear_brun_called(void){brun_called=0;}
		inline void Clear_evnt_called(void){evnt_called=0;}
		inline void Clear_erun_called(void){erun_called=0;}
		inline void Clear_fini_called(void){fini_called=0;}

		inline void Set_init_called(void){init_called=1;}
		inline void Set_brun_called(void){brun_called=1;}
		inline void Set_evnt_called(void){evnt_called=1;}
		inline void Set_erun_called(void){erun_called=1;}
		inline void Set_fini_called(void){fini_called=1;}
		inline void SetBRUN_RunNumber(int run){brun_runnumber = run;}
		
		inline void LockState(void){pthread_mutex_lock(&state_mutex);}
		inline void UnlockState(void){pthread_mutex_unlock(&state_mutex);}
		inline void SetJApplication(JApplication *app){this->app = app;}
		inline void SetDeleteMe(bool delete_me){this->delete_me=delete_me;} ///< If true, this JEventProcessor object will be deleted when JApplication::fini is called at the end of Run. Default value is false.
		inline bool GetDeleteMe(void){return delete_me;} ///< Returns current state of delete_me flag. 

	protected:
		JApplication *app;
		int init_called;
		int brun_called;
		int evnt_called;
		int erun_called;
		int fini_called;
		int brun_runnumber;
		int brun_eventnumber;
		bool delete_me;
	
	private:
		pthread_mutex_t state_mutex;

};

} // Close JANA namespace

#endif // _JEventProcessor_

