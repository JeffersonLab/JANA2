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

#define jout std::cout
#define jerr std::cerr

#include <JANA/JParameterManager.h>
#include <JANA/JLogger.h>
#include <JANA/JResourcePool.h>
#include <JANA/JResourcePoolSimple.h>

class JApplication;
class JEventProcessor;
class JEventSource;
class JEventSourceGenerator;
class JFactoryGenerator;
class JCalibrationGenerator;
class JQueue;
class JThread;
class JEventSourceManager;
class JThreadManager;
class JFactorySet;

template <typename ReturnType>
class JTask;

extern JApplication *japp;

//////////////////////////////////////////////////////////////////////////////////////////////////
/// JANA application class (singleton).
///
/// The JApplication class serves as a central access point for getting to most things
/// in the JANA application. It owns the JThreadManager, JParameterManager, etc.
/// It is also responsible for making sure all of the plugins are attached and other
/// user specified configurations for the run are implemented before starting the processing
/// of the data. User code (e.g. plugins) will generally register things like event sources
/// and processors with the JApplication so they can be called up later at the appropriate time.
//////////////////////////////////////////////////////////////////////////////////////////////////
class JApplication{



	public:
	
		enum RETURN_STATUS{
			kSUCCESS,
			kNO_MORE_EVENTS,
			kTRY_AGAIN,
			kUNKNOWN
		};

    JApplication(JParameterManager* params = nullptr, std::vector<string>* eventSources = nullptr);
		virtual ~JApplication();

		void AddSignalHandlers(void);
		int  GetExitCode(void);
		void Initialize(void);
		void PrintFinalReport(void);
		void PrintStatus(void);
		void Quit(bool skip_join=false);
		void Run(uint32_t nthreads=0);
		void SetExitCode(int exit_code);
		void SetMaxThreads(uint32_t);
		void SetTicker(bool ticker_on=true);
		void Stop(bool wait_until_idle=false);
		void Resume(void);

		void Add(JEventSourceGenerator *source_generator);
		void Add(JFactoryGenerator *factory_generator);
		void Add(JEventProcessor *processor);

		void AddPlugin(string plugin_name);
		void AddPluginPath(string path);
		
		void GetJEventProcessors(vector<JEventProcessor*>& aProcessors);
		void GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators);
		std::shared_ptr<JLogger> GetJLogger(void);
		JParameterManager* GetJParameterManager(void);
		JThreadManager* GetJThreadManager(void) const;
		JEventSourceManager* GetJEventSourceManager(void) const;
		
		//GET/RECYCLE POOL RESOURCES
		std::shared_ptr<JTask<void>> GetVoidTask(void);
		JFactorySet* GetFactorySet(void);
		void Recycle(JFactorySet* aFactorySet);
		void UpdateResourceLimits(void);

		uint64_t GetNtasksCompleted(string name="");
		uint64_t GetNeventsProcessed(void);
		float GetIntegratedRate(void);
		float GetInstantaneousRate(void);
		void GetInstantaneousRates(vector<double> &rates_by_queue);
		void GetIntegratedRates(map<string,double> &rates_by_thread);
	
		bool IsQuitting(void){ return _quitting; }
		bool IsDrainingQueues(void){ return _draining_queues; }

		void RemoveJEventProcessor(JEventProcessor *processor);
		void RemoveJFactoryGenerator(JFactoryGenerator *factory_generator);
		void RemovePlugin(string &plugin_name);

		string Val2StringWithPrefix(float val);
		template<typename T> T GetParameterValue(std::string name);
		template<typename T> JParameter* SetParameterValue(std::string name, T val);
	
	protected:
	
		int _exit_code;
		bool _skip_join;
		bool _quitting;
		int _verbose;
		bool _draining_queues;
		bool _ticker_on;
		std::chrono::time_point<std::chrono::high_resolution_clock> mRunStartTime;
		std::vector<string> _plugins;
		std::vector<string> _plugin_paths;
		std::vector<void*> _sohandles;
		std::vector<JFactoryGenerator*> _factoryGenerators;
		std::vector<JCalibrationGenerator*> _calibrationGenerators;
		std::vector<JEventProcessor*> _eventProcessors;

		std::shared_ptr<JLogger> _logger;
		JParameterManager *_pmanager;
		JEventSourceManager* _eventSourceManager;
		JThreadManager* _threadManager;
		std::size_t mNumProcessorsAdded;

		void AttachPlugins(void);
		void AttachPlugin(string name, bool verbose=false);
		
	private:

		// Resource pools
		// TODO: Add methods to set control parameters
		JResourcePool<JTask<void>> mVoidTaskPool;
		JResourcePoolSimple<JFactorySet> mFactorySetPool;

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

//---------------------------------
// SetParameterValue
//---------------------------------
template<typename T>
JParameter* JApplication::SetParameterValue(std::string name, T val)
{
	/// This is a convenience function that just calls the SetParameter
	/// of JParameterManager.
	return GetJParameterManager()->SetParameter(name, val);
}


// This routine is used to bootstrap plugins. It is done outside
// of the JApplication class to ensure it sees the global variables
// that the rest of the plugin's InitPlugin routine sees.
inline void InitJANAPlugin(JApplication *app)
{
	// Make sure global pointers are pointing to the 
	// same ones being used by executable
	japp = app;
}

#endif // _JApplication_h_

