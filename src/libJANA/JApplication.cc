//
//    File: JApplication.cc
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

#include "JApplication.h"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JQueue.h>
#include <JANA/JParameterManager.h>
#include <JANA/JResourceManager.h>
#include <JANA/JThread.h>
#include <JANA/JTask.h>


//---------------------------------
// JApplication    (Constructor)
//---------------------------------
JApplication::JApplication()
{

}

//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{

}

//---------------------------------
// GetExitCode
//---------------------------------
int JApplication::GetExitCode(void)
{
	
}

//---------------------------------
// Quit
//---------------------------------
void JApplication::Quit(void)
{
	
}

//---------------------------------
// Run
//---------------------------------
void JApplication::Run(uint32_t nthreads)
{
	
}

//---------------------------------
// SetMaxThreads
//---------------------------------
void JApplication::SetMaxThreads(uint32_t)
{
	
}

//---------------------------------
// SetTicker
//---------------------------------
void JApplication::SetTicker(bool ticker_on)
{
	
}

//---------------------------------
// Stop
//---------------------------------
void JApplication::Stop(void)
{
	
}

//---------------------------------
// AddJEventProcessor
//---------------------------------
void JApplication::AddJEventProcessor(JEventProcessor *processor)
{
	
}

//---------------------------------
// AddJEventSource
//---------------------------------
void JApplication::AddJEventSource(JEventSource *source)
{
	
}

//---------------------------------
// AddJEventSourceGenerator
//---------------------------------
void JApplication::AddJEventSourceGenerator(JEventSourceGenerator *source_generator)
{
	
}

//---------------------------------
// AddJFactoryGenerator
//---------------------------------
void JApplication::AddJFactoryGenerator(JFactoryGenerator *factory_generator)
{
	
}

//---------------------------------
// GetJEventProcessors
//---------------------------------
void JApplication::GetJEventProcessors(vector<JEventProcessor*> &processors)
{
	
}

//---------------------------------
// GetJEventSources
//---------------------------------
void JApplication::GetJEventSources(vector<JEventSource*> &sources)
{
	
}

//---------------------------------
// GetJEventSourceGenerators
//---------------------------------
void JApplication::GetJEventSourceGenerators(vector<EventSourceGenerator*> &source_generators)
{
	
}

//---------------------------------
// GetJFactoryGenerators
//---------------------------------
void JApplication::GetJFactoryGenerators(vector<JFactoryGenerator*> &factory_generators)
{
	
}

//---------------------------------
// GetJQueues
//---------------------------------
void JApplication::GetJQueues(vector<const JQueue*> &queues)
{
	
}

//---------------------------------
// GetJQueue
//---------------------------------
const JQueue* JApplication::GetJQueue(const string &name)
{
	
}

//---------------------------------
// GetJParameterManager
//---------------------------------
const JParameterManager* JApplication::GetJParameterManager(void)
{
	
}

//---------------------------------
// GetJResourceManager
//---------------------------------
const JResourceManager* JApplication::GetJResourceManager(void)
{
	
}

//---------------------------------
// GetNcores
//---------------------------------
uint32_t JApplication::GetNcores(void)
{
	
}

//---------------------------------
// GetNJThreads
//---------------------------------
uint32_t JApplication::GetNJThreads(void)
{
	
}

//---------------------------------
// GetNtasksCompleted
//---------------------------------
uint64_t JApplication::GetNtasksCompleted(string name="")
{
	
}

//---------------------------------
// GetNeventsProcessed
//---------------------------------
uint64_t JApplication::GetNeventsProcessed(void)
{
	
}

//---------------------------------
// GetIntegratedRate
//---------------------------------
float JApplication::GetIntegratedRate(void)
{
	
}

//---------------------------------
// GetInstantaneousRate
//---------------------------------
float JApplication::GetInstantaneousRate(void)
{
	
}

//---------------------------------
// GetInstantaneousRates
//---------------------------------
void JApplication::GetInstantaneousRates(vector<double> &rates_by_queue)
{
	
}

//---------------------------------
// GetIntegratedRates
//---------------------------------
void JApplication::GetIntegratedRates(map<string,double> &rates_by_thread)
{
	
}

//---------------------------------
// RemoveJEventProcessor
//---------------------------------
void JApplication::RemoveJEventProcessor(JEvenetProcessor *processor)
{
	
}

//---------------------------------
// RemoveJEventSource
//---------------------------------
void JApplication::RemoveJEventSource(JEventSource *source)
{
	
}

//---------------------------------
// RemoveJEventSourceGenerator
//---------------------------------
void JApplication::RemoveJEventSourceGenerator(JEventSourceGenerator *source_generator)
{
	
}

//---------------------------------
// RemoveJFactoryGenerator
//---------------------------------
void JApplication::RemoveJFactoryGenerator(JFactoryGenerator *factory_generator)
{
	
}
