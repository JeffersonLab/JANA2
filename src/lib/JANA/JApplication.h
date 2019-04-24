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
#include <memory>
#include <map>

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
	
    JApplication(JParameterManager* params = nullptr, std::vector<string>* eventSources = nullptr);
		virtual ~JApplication();

		int  GetExitCode(void);
		virtual void Initialize(void) = 0;
		virtual void PrintFinalReport(void) = 0;
		virtual void PrintStatus(void);
		virtual void Quit(bool skip_join=false) = 0;
		virtual void Run() = 0;
		virtual void Scale(int nthreads) = 0;
		virtual void Stop(bool wait_until_idle=false) = 0;
		virtual void Resume() = 0;
		void SetExitCode(int exit_code);
		void SetTicker(bool ticker_on=true);

		virtual void Add(JEventSourceGenerator *source_generator);
		virtual void Add(JFactoryGenerator *factory_generator);
		virtual void Add(JEventProcessor *processor);

		void AddPlugin(string plugin_name);
		void AddPluginPath(string path);
		
		void GetJEventProcessors(vector<JEventProcessor*>& aProcessors);
		void GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators);
		JParameterManager* GetJParameterManager(void);
		virtual JThreadManager* GetJThreadManager(void) const = 0;
		JEventSourceManager* GetJEventSourceManager(void) const;
		
		//GET/RECYCLE POOL RESOURCES
		virtual void UpdateResourceLimits(void) = 0;
		virtual std::shared_ptr<JTask<void>> GetVoidTask(void) = 0;
		JFactorySet* GetFactorySet(void);
		void Recycle(JFactorySet* aFactorySet);

		uint64_t GetNThreads();
		virtual uint64_t GetNeventsProcessed(void);
		float GetIntegratedRate();
		float GetInstantaneousRate();

		bool IsQuitting(void){ return _quitting; }
		bool IsDrainingQueues(void){ return _draining_queues; }

		string Val2StringWithPrefix(float val);
		template<typename T> T GetParameterValue(std::string name);
		template<typename T> JParameter* SetParameterValue(std::string name, T val);
	
	protected:

		bool _initialized = false;
		int _exit_code;
		bool _ticker_on;
		std::chrono::time_point<std::chrono::high_resolution_clock> mRunStartTime;

		JLogger _logger;
		JParameterManager *_pmanager;

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

