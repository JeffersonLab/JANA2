//
//    File: JEventSourceManager.cc
// Created: Wed Oct 11 22:51:22 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
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

#include <algorithm>

#include "JEventSourceManager.h"
#include "JEventSource.h"

using namespace std;

//---------------------------------
// Constructor
//---------------------------------
JEventSourceManager::JEventSourceManager(JApplication* aApp) : mApplication(aApp)
{

}

//---------------------------------
// AddEventSource
//---------------------------------
void JEventSourceManager::AddEventSource(const std::string& source_name)
{
	/// Add the named event source to the list of event sources to read from.
	/// Usually, these are taken from the command line, but this allows cutom
	/// plugins to add a source programmatically. This will be added to the end
	/// of the current list of source names (i.e. after any entered on the
	/// command line.)

	_source_names.push_back(source_name);
	_source_names_unopened.push_back(source_name);
}

//---------------------------------
// AddJEventSource
//---------------------------------
void JEventSourceManager::AddJEventSource(JEventSource *source)
{

}

//---------------------------------
// AddJEventSourceGenerator
//---------------------------------
void JEventSourceManager::AddJEventSourceGenerator(JEventSourceGenerator *source_generator)
{
	/// Add the given JEventSourceGenerator to the list of queues
	_eventSourceGenerators.push_back( source_generator );
}

//---------------------------------
// GetJEventSources
//---------------------------------
void JEventSourceManager::GetJEventSources(std::vector<JEventSource*>& aSources) const
{
	aSources = _sources_active;
}

//---------------------------------
// GetJEventSourceGenerators
//---------------------------------
void JEventSourceManager::GetJEventSourceGenerators(std::vector<JEventSourceGenerator*>& aGenerators) const
{
	aGenerators = _eventSourceGenerators;
}

//---------------------------------
// ClearJEventSourceGenerators
//---------------------------------
void JEventSourceManager::ClearJEventSourceGenerators(void)
{
	_eventSourceGenerators.clear();
}

//---------------------------------
// OpenNext
//---------------------------------
void JEventSourceManager::OpenInitSources(void)
{
	//Single-threaded here!
	if(_source_names_unopened.empty())
		return;

	auto sNumFilesToOpen = (_source_names_unopened.size() >= mMaxNumOpenFiles) ? mMaxNumOpenFiles : _source_names_unopened.size();
	for(std::size_t si = 0; si < sNumFilesToOpen; si++)
	{
		std::string source_name = _source_names_unopened.front();
		_source_names_unopened.pop_front();
		_sources_active[si] = OpenSource(source_name);
	}
}

//---------------------------------
// OpenSource
//---------------------------------
JEventSource* JEventSourceManager::OpenSource(const std::string& source_name)
{
	// Check if the user has forced a specific type of event source
	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
	// search for that source and use it. Otherwise, throw an exception.
	auto gen = GetUserEventSourceGenerator();

	if(gen == nullptr)
		gen = GetEventSourceGenerator(source_name);

	// Try opening the source using the chosen generator
	JEventSource *new_source = nullptr;
	if(gen != nullptr){
		jout << "Opening source \"" << source_name << "\" of type: "<< gen->Description() << endl;
		new_source = gen->MakeJEventSource(source_name);
	}

	if(new_source)
		return new_source;

	// Problem opening source. Notify user
	jerr<<std::endl;
	jerr<<"  xxxxxxxxxxxx  Unable to open event source \""<<source_name<<"\"!  xxxxxxxxxxxx"<<std::endl;
	jerr<<std::endl;
	if( _source_names_unopened.empty() ){
		unsigned int Nactive_sources = 0;
		for(auto src : _sources_active) if( src != nullptr ) Nactive_sources++;
		if( (Nactive_sources==0) && (_sources_exhausted.empty()) ){
			jerr<<"   xxxxxxxxxxxx  NO VALID EVENT SOURCES GIVEN !!!   xxxxxxxxxxxx  "<<std::endl;
			jerr<<std::endl;
			mApplication->SetExitCode(-1);
			mApplication->Quit();
		}
	}

	return nullptr;
}

//---------------------------------
// OpenNext
//---------------------------------
std::pair<JEventSource::RETURN_STATUS, JEventSource*> JEventSourceManager::OpenNext(const JEventSource* aPreviousSource)
{
	/// Remove the previous source from the opened list and open a new one.
	auto sFindSlot = [aPreviousSource](const JEventSource* sSource) -> bool { return (sSource == aPreviousSource); };

	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);

	//Find slot
	auto sEnd = std::end(_sources_active);
	auto sIterator = std::find_if(std::begin(_sources_active), sEnd, sFindSlot);
	if(sIterator == sEnd)
		return std::make_pair(JEventSource::kSUCCESS, (JEventSource*)nullptr); //Previous source already closed and new one already opened (if any)

	//Register source finished
	_sources_exhausted.push_back(*sIterator);

	if( _source_names_unopened.empty())
		return std::make_pair(JEventSource::kNO_MORE_EVENTS, (JEventSource*)nullptr);

	// Get name of next source to open
	auto source_name = _source_names_unopened.front();
	_source_names_unopened.pop_front();

	*sIterator = OpenSource(source_name);
	return std::make_pair(JEventSource::kSUCCESS, *sIterator);
}

//---------------------------------
// RemoveJEventSource
//---------------------------------
void JEventSourceManager::RemoveJEventSource(JEventSource *source)
{

}


//---------------------------------
// RemoveJEventSourceGenerator
//---------------------------------
void JEventSourceManager::RemoveJEventSourceGenerator(JEventSourceGenerator *source_generator)
{

}

//---------------------------------
// GetUserEventSourceGenerator
//---------------------------------
JEventSourceGenerator* JEventSourceManager::GetEventSourceGenerator(const std::string& source_name)
{
	// Loop over JEventSourceGenerator objects and find the one
	// (if any) that has the highest chance of being able to read
	// this source. The return value of
	// JEventSourceGenerator::CheckOpenable(source) is a liklihood that
	// the named source can be read by the JEventSource objects
	// created by the generator. In most cases, the liklihood will
	// be either 0.0 or 1.0. In the case that 2 or more generators return
	// equal liklihoods, the first one in the list will be used.
	JEventSourceGenerator* gen = nullptr;
	double liklihood = 0.0;
	for( auto sg : _eventSourceGenerators ){
		double my_liklihood = sg->CheckOpenable(source_name);
		if(my_liklihood > liklihood){
			liklihood = my_liklihood;
			gen = sg;
		}
	}
	return gen;
}

//---------------------------------
// GetUserEventSourceGenerator
//---------------------------------
JEventSourceGenerator* JEventSourceManager::GetUserEventSourceGenerator(void)
{
	// Check if the user has forced a specific type of event source
	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
	// search for that source and use it. Otherwise, throw an exception.

	JEventSourceGenerator* gen = nullptr;
	try{
		std::string EVENT_SOURCE_TYPE = mApplication->GetParameterValue<string>("EVENT_SOURCE_TYPE");
		for( auto sg : _eventSourceGenerators ){
			if( sg->GetName() == EVENT_SOURCE_TYPE ){
				gen = sg;
				jout << "Forcing use of event source type: " << EVENT_SOURCE_TYPE << endl;
				break;
			}
		}
		if(!gen){
			jerr << endl;
			jerr << "-----------------------------------------------------------------" << endl;
			jerr << " You specified event source type \"" << EVENT_SOURCE_TYPE << "\"" << endl;
			jerr << " be used to read the event sources but no such type exists." << endl;
			jerr << " Here is a list of available source types:" << endl;
			jerr << endl;
			for( auto sg : _eventSourceGenerators ) jerr << "   " << sg->GetName() << endl;
			jerr << endl;
			jerr << "-----------------------------------------------------------------" << endl;
			mApplication->SetExitCode(-1);
			mApplication->Quit();
		}
	}catch(...){}

	return gen;
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
std::size_t JEventSourceManager::GetNumEventsProcessed(void) const
{
	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);

	auto sNumEventGetter = [](JEventSource* aSource) -> std::size_t {return aSource->GetNumEventsProcessed();};

	//Sum from active sources and exhausted ones
	auto sNumEvents = std::accumulate(std::begin(_sources_active), std::end(_sources_active), 0, sNumEventGetter);
	return std::accumulate(std::begin(_sources_exhausted), std::end(_sources_exhausted), sNumEvents, sNumEventGetter);
}

//---------------------------------
// AreAllFilesClosed
//---------------------------------
bool JEventSourceManager::AreAllFilesClosed(void) const
{
	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);

	if(!_source_names_unopened.empty())
		return false;

	auto sClosedChecker = [](JEventSource* aSource) -> bool {return aSource->IsFileClosed();};
	return std::all_of(std::begin(_sources_active), std::end(_sources_active), sClosedChecker);
}
