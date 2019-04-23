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

#include <unistd.h>
#include <sched.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_set>
using namespace std;

#include "JApplication.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JParameterManager.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JException.h>
#include <JANA/JVersion.h>
#include <JANA/JResourcePool.h>
#include <JANA/JResourcePoolSimple.h>
#include <JANA/JCpuInfo.h>

JApplication *japp = nullptr;


//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication(JParameterManager* params,
						   std::vector<string>* eventSources)
{
	_exit_code = 0;
	_ticker_on = true;
	mNumProcessorsAdded = 0;

	_pmanager = (params == nullptr) ? new JParameterManager() : params;
	_logger = JLoggingService::logger("JApplication");
	_eventSourceManager = new JEventSourceManager(this);

	if (eventSources != nullptr) {
		for (string & e : *eventSources) {
			LOG_INFO(_logger) << "Adding source: " << e << "\n" << LOG_END;
			_eventSourceManager->AddEventSource(e);
		}
	}
}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{
	for( auto p: _factoryGenerators     ) delete p;
	for( auto p: _eventProcessors       ) delete p;
	if( _pmanager           ) delete _pmanager;
	if( _eventSourceManager ) delete _eventSourceManager;
}

void JApplication::AddPlugin(string plugin_name)
{

void JApplication::AddPluginPath(string path)
{
}

int JApplication::GetExitCode(void)
{
	/// Returns the currently set exit code. This can be used by
	/// JProcessor/JFactory classes to communicate an appropriate
	/// exit code that a jana program can return upon exit. The
	/// value can be set via the SetExitCode method.

	return _exit_code;
}


void JApplication::PrintStatus(void)
{
	// Print ticker
	stringstream ss;
	ss << "  " << GetNeventsProcessed() << " events processed  " << Val2StringWithPrefix( GetInstantaneousRate() ) << "Hz (" << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz avg)             ";
	jout << ss.str() << "\r";
	jout.flush();
}

void JApplication::SetExitCode(int exit_code)
{
	/// Set a value of the exit code in that can be later retrieved
	/// using GetExitCode. This is so the executable can return
	/// a meaningful error code if processing is stopped prematurely,
	/// but the program is able to stop gracefully without a hard 
	/// exit. See also GetExitCode.

	_exit_code = exit_code;
}

void JApplication::SetTicker(bool ticker_on)
{
	_ticker_on = ticker_on;
}

void JApplication::Add(JEventSourceGenerator *source_generator)
{
	/// Add the given JFactoryGenerator to the list of queues
	///
	/// @param source_generator pointer to source generator to add. Ownership is passed to JApplication

}

void JApplication::Add(JFactoryGenerator *factory_generator)
{
	/// Add the given JFactoryGenerator to the list of queues
	///
	/// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication

	_factoryGenerators.push_back( factory_generator );
}


void JApplication::Add(JEventProcessor *processor)
{
	mNumProcessorsAdded++;
}

void JApplication::GetJEventProcessors(vector<JEventProcessor*>& aProcessors)
{
	aProcessors = _eventProcessors;
}

void JApplication::GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators)
{
	factory_generators = _factoryGenerators;
}


JParameterManager* JApplication::GetJParameterManager(void)
{
	/// Return pointer to the JParameterManager object.

	if( !_pmanager ) _pmanager = new JParameterManager();

	return _pmanager;
}


JEventSourceManager* JApplication::GetJEventSourceManager(void) const
{
	return _eventSourceManager;
}



JFactorySet* JApplication::GetFactorySet(void)
{
	return mFactorySetPool.Get_Resource(_factoryGenerators);
}

void JApplication::Recycle(JFactorySet* aFactorySet)
{
	return mFactorySetPool.Recycle(aFactorySet);
}

uint64_t JApplication::GetNThreads() {
	return _nthreads;
}

uint64_t JApplication::GetNtasksCompleted(string name)
{
	return 0;
}

uint64_t JApplication::GetNeventsProcessed(void)
{
	/// Return the total number of events processed.
	return _eventSourceManager->GetNumEventsProcessed();
}

float JApplication::GetIntegratedRate(void)
{
	/// Returns the total integrated rate so far in Hz since
	/// Run was called.

	static float last_R = 0.0;

	auto now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> delta_t = now - mRunStartTime;
	float delta_t_seconds = delta_t.count();
	float delta_N = (float)GetNeventsProcessed();

	if( delta_t_seconds >= 0.5) {
		last_R = delta_N / delta_t_seconds;
	}

	return last_R;
}

float JApplication::GetInstantaneousRate(void)
{
	auto now = std::chrono::high_resolution_clock::now();
	uint64_t N = GetNeventsProcessed();
	static auto last_t = now;
	static uint64_t last_N = N;

	std::chrono::duration<float> delta_t = now - last_t;
	float delta_t_seconds = delta_t.count();
	float delta_N = (float)(GetNeventsProcessed() - last_N);

	static float last_R = 0.0;
	if( delta_t_seconds >= 0.5) {
		last_R = delta_N / delta_t_seconds;
		last_t = now;
		last_N = N;
	}

	return last_R;
}

string JApplication::Val2StringWithPrefix(float val)
{
	/// Return the value as a string with the appropriate latin unit prefix
	/// appended.
	/// Values returned are: "G", "M", "k", "", "u", and "m" for
	/// values of "val" that are: >1.5E9, >1.5E6, >1.5E3, <1.0E-7, <1.0E-4, 1.0E-1
	/// respectively.
	const char *units = "";
	if(val>1.5E9){
		val/=1.0E9;
		units = "G";
	}else 	if(val>1.5E6){
		val/=1.0E6;
		units = "M";
	}else if(val>1.5E3){
		val/=1.0E3;
		units = "k";
	}else if(val<1.0E-7){
		units = "";
	}else if(val<1.0E-4){
		val/=1.0E6;
		units = "u";
	}else if(val<1.0E-1){
		val/=1.0E3;
		units = "m";
	}

	char str[256];
	sprintf(str,"%3.1f %s", val, units);

	return string(str);
}

