//
//    File: JApplication.h
// Created: Wed Oct 11 13:09:35 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#ifndef _JApplication_h_
#define _JApplication_h_

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <atomic>
#include <deque>
#include <mutex>
#include <memory>
#include <map>
using std::vector;
using std::string;
using std::deque;
using std::map;

#ifndef _DBG__
#include <mutex>
extern std::mutex DBG_MUTEX;
//#define _DBG_LOCK_ DBG_MUTEX.lock()
//#define _DBG_RELEASE_ DBG_MUTEX.unlock()
//#define _DBG__ std::cerr<<__FILE__<<":"<<__LINE__<<std::endl
//#define _DBG_  std::cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG_ {DBG_MUTEX.lock();std::cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG_ENDL_ std::endl;DBG_MUTEX.unlock();}
#define _DBG__ _DBG_<<_DBG_ENDL_
#endif

#define jout std::cout
#define jerr std::cerr

#include "JResourcePool.h"
#include <JANA/JParameterManager.h>

class JApplication;
class JEventProcessor;
class JEventSource;
class JEventSourceGenerator;
class JFactoryGenerator;
class JCalibrationGenerator;
class JQueue;
class JParameterManager;
class JResourceManager;
class JThread;
class JEventSourceManager;
class JThreadManager;
class JFactorySet;
class JLogWrapper;

template <typename ReturnType>
class JTask;

extern JApplication *japp;


class JApplication{
	public:
	
		enum RETURN_STATUS{
			kSUCCESS,
			kNO_MORE_EVENTS,
			kTRY_AGAIN,
			kUNKNOWN
		};
	
		JApplication(int narg, char *argv[]);
		virtual ~JApplication();

		void AddSignalHandlers(void);
		int  GetExitCode(void);
		void Initialize(void);
		void PrintFinalReport(void);
		void PrintStatus(void);
		void Quit(void);
		void Run(uint32_t nthreads=0);
		void SetExitCode(int exit_code);
		void SetMaxThreads(uint32_t);
		void SetTicker(bool ticker_on=true);
		void Stop(void);
		
		void Add(JEventSourceGenerator *source_generator);
		void Add(JFactoryGenerator *factory_generator);
		void Add(JEventProcessor *processor);

		void AddPlugin(string plugin_name);
		void AddPluginPath(string path);
		
		void GetJEventProcessors(vector<JEventProcessor*>& aProcessors);
		void GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators);
		JParameterManager* GetJParameterManager(void);
		JResourceManager* GetJResourceManager(void);
		JThreadManager* GetJThreadManager(void) const;
		JEventSourceManager* GetJEventSourceManager(void) const;
		
		//GET/RECYCLE POOL RESOURCES
		std::shared_ptr<JTask<void>> GetVoidTask(void);
		JFactorySet* GetFactorySet(void);
		void Recycle(JFactorySet* aFactorySet);

		uint32_t GetCPU(void);
		uint64_t GetNtasksCompleted(string name="");
		uint64_t GetNeventsProcessed(void);
		float GetIntegratedRate(void);
		float GetInstantaneousRate(void);
		void GetInstantaneousRates(vector<double> &rates_by_queue);
		void GetIntegratedRates(map<string,double> &rates_by_thread);
		
		void RemoveJEventProcessor(JEventProcessor *processor);
		void RemoveJFactoryGenerator(JFactoryGenerator *factory_generator);
		void RemovePlugin(string &plugin_name);

		string Val2StringWithPrefix(float val);
		template<typename T> T GetParameterValue(std::string name);

		//LOG WRAPPERS
		JLogWrapper* GetLogWrapper(uint32_t aLogIndex) const;
		void SetLogWrapper(uint32_t aLogIndex, JLogWrapper* aLogWrapper);

	protected:
	
		vector<string> _args;	///< Argument list passed in to JApplication Constructor
		int _exit_code;
		bool _quitting;
		bool _draining_queues;
		bool _ticker_on;
		std::vector<string> _plugins;
		std::vector<string> _plugin_paths;
		std::vector<void*> _sohandles;
		std::vector<JThread*> _jthreads;
		std::vector<JFactoryGenerator*> _factoryGenerators;
		std::vector<JCalibrationGenerator*> _calibrationGenerators;
		std::vector<JEventProcessor*> _eventProcessors;
		JParameterManager *_pmanager;
		JResourceManager *_rmanager;
		JEventSourceManager* _eventSourceManager;
		JThreadManager* _threadManager;
		std::map<uint32_t, JLogWrapper*> mLogWrappers;
	
		void AttachPlugins(void);
		void AttachPlugin(string name, bool verbose=false);
		
	private:

		//Resource pools
		//TODO: Add methods to set control parameters
		JResourcePool<JTask<void>> mVoidTaskPool;
		JResourcePool<JFactorySet> mFactorySetPool;

};

//---------------------------------
// GetParameterValue
//---------------------------------
template<typename T>
T JApplication::GetParameterValue(std::string name)
{	
	/// This is a convenience function that just calls the method
	/// of the same name in JParameterManager.	
	return GetJParameterManager()->GetParameterValue<T>(name);
}		


// This routine is used to bootstrap plugins. It is done outside
// of the JApplication class to ensure it sees the global variables
// that the rest of the plugin's InitPlugin routine sees.
inline void InitJANAPlugin(JApplication *app)
{
	// Make sure global pointers are pointing to the 
	// same ones being used by executable
	japp = app;
	gPARMS = app->GetJParameterManager();
}

#endif // _JApplication_h_

