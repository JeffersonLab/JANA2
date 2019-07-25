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
#include <numeric>

#include "JEventSourceManager.h"
#include "JEventSource.h"

using namespace std;

//---------------------------------
// Constructor
//---------------------------------
JEventSourceManager::JEventSourceManager(JApplication* aApp) : mApplication(aApp)
{
	aApp->GetJParameterManager()->SetDefaultParameter(
		"JANA:MAX_NUM_OPEN_SOURCES", 
		mMaxNumOpenFiles, 
		"Max # input sources that can be open simultaneously");
}

//---------------------------------
// Destructor
//---------------------------------
JEventSourceManager::~JEventSourceManager()
{
	for(auto p : _eventSourceGenerators) delete p;
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
}

//---------------------------------
// AddJEventSource
//---------------------------------
void JEventSourceManager::AddJEventSource(JEventSource *source)
{
	_sources_unopened.push_back(source);
}

//---------------------------------
// AddJEventSourceGenerator
//---------------------------------
void JEventSourceManager::AddJEventSourceGenerator(JEventSourceGenerator *source_generator)
{
	/// Add the given JEventSourceGenerator to the list of queues
	source_generator->SetJApplication( mApplication );
	_eventSourceGenerators.push_back( source_generator );
}

//---------------------------------
// GetActiveJEventSources
//---------------------------------
void JEventSourceManager::GetActiveJEventSources(std::vector<JEventSource*>& aSources) const
{
	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);
	aSources = _sources_active;
}

//---------------------------------
// GetUnopenedJEventSources
//---------------------------------
void JEventSourceManager::GetUnopenedJEventSources(std::deque<JEventSource*>& aSources) const
{
	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);
	aSources = _sources_unopened;
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
// CreateSources
//---------------------------------
void JEventSourceManager::CreateSources(void)
{
	//Single-threaded here!

	//Create all of the JEventSource's that we will need, even if they won't all be active at the same time.
	//We will have a separate method to open the sources that we'll have to call before reading from them.

	//Why do we do this?
	//We need to store the objects in the event sources in factories, and these types are only known to the source.
	//These factories need to be created for these sources, and stored in the JFactorySet
	//However, we want this operation to happen at the beginning of the program, while we're still single-threaded.
	//If we instead do it on-demand during JEvent analysis, we'd have to lock the JFactorySet every time we add/get a factory from it.

	for(auto sSourceName : _source_names)
	{
		auto sSource = CreateSource(sSourceName);
		if(sSource != nullptr)
			_sources_unopened.push_back(sSource);
	}

	if(_sources_unopened.empty())
	{
		jerr<<"   xxxxxxxxxxxx  NO VALID EVENT SOURCES GIVEN !!!   xxxxxxxxxxxx  "<<std::endl;
		jerr<<std::endl;
		mApplication->SetExitCode(-1);
		mApplication->Quit();
	}
}

//---------------------------------
// OpenInitSources
//---------------------------------
void JEventSourceManager::OpenInitSources(void)
{
	//Single-threaded here!
	if(_sources_unopened.empty())
		return;

	//Open up to the max allowed number of simultaneously-open sources, and register them as active.
	auto sNumFilesToOpen = (_sources_unopened.size() >= mMaxNumOpenFiles) ? mMaxNumOpenFiles : _sources_unopened.size();
	for(std::size_t si = 0; si < sNumFilesToOpen; si++)
	{
		auto sSource = _sources_unopened.front();
		_sources_unopened.pop_front();
		try {
			std::call_once(sSource->mOpened, [&](){ sSource->Open(); });
		}
		catch (JException& e) {
			e.plugin_name = sSource->GetPlugin();
			e.component_name = sSource->GetType();
			throw e;
		}
		_sources_active.push_back(sSource);
	}
}

//---------------------------------
// CreateSource
//---------------------------------
JEventSource* JEventSourceManager::CreateSource(const std::string& source_name)
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
		jout << "Opening source \"" << source_name << "\" - "<< gen->GetType() << " : " << gen->GetDescription() << endl;
		new_source = gen->MakeJEventSource(source_name);
	}

	if(new_source != nullptr){
		new_source->SetPlugin(gen->GetPlugin());
		new_source->SetJApplication(mApplication);
		_sources_allocated.push_back(std::shared_ptr<JEventSource>(new_source)); // ensure destruction
		return new_source;
	}

	// Problem opening source. Notify user
	jerr<<std::endl;
	jerr<<"  xxxxxxxxxxxx  Unable to open event source \""<<source_name<<"\"!  xxxxxxxxxxxx"<<std::endl;
	jerr<<std::endl;

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

	//Check if a previous thread has already closed the last source
	if(_sources_active.empty())
		return std::make_pair(JEventSource::RETURN_STATUS::kNO_MORE_EVENTS, (JEventSource*)nullptr);

	//Find slot
	auto sEnd = std::end(_sources_active);
	auto sIterator = std::find_if(std::begin(_sources_active), sEnd, sFindSlot);
	if(sIterator == sEnd) //Previous source already closed and new one already opened (if any)
		return std::make_pair(JEventSource::RETURN_STATUS::kUNKNOWN, (JEventSource*)nullptr); //unknown: don't know if any were opened

	//Register source finished
	_sources_exhausted.push_back(*sIterator);
	_sources_active.erase(sIterator);

	//If no new sources to open, return
	if(_sources_unopened.empty())
		return std::make_pair(JEventSource::RETURN_STATUS::kNO_MORE_EVENTS, (JEventSource*)nullptr);

	//Get the new source
	auto sNewSource = _sources_unopened.front();
	_sources_unopened.pop_front();

	//Open the new source, register it, and return it
	try {
		std::call_once(sNewSource->mOpened, [&](){ sNewSource->Open(); });
	}
	catch (JException& e) {
		e.component_name = sNewSource->GetType();
		e.plugin_name = sNewSource->GetPlugin();
		throw e;
	}
	_sources_active.push_back(sNewSource);
	return std::make_pair(JEventSource::RETURN_STATUS::kSUCCESS, sNewSource);
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
// GetEventSourceGenerator
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
JEventSourceGenerator* JEventSourceManager::GetUserEventSourceGenerator(void) {
	// Check if the user has forced a specific type of event source
	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
	// search for that source and use it. Otherwise, return nullptr.

	std::string event_source_type;

	// If no EVENT_SOURCE_TYPE specified, return nullptr
	try {
		event_source_type = mApplication->GetParameterValue<string>("EVENT_SOURCE_TYPE");
	} catch (...) {
		return nullptr;
	}

	// If corresponding JEventSourceGenerator found, use that one
	for (auto sg : _eventSourceGenerators) {
		if (sg->GetType() == event_source_type) {
			jout << "Forcing use of event source type: " << event_source_type << endl;
			return sg;
		}
	}

	// If no corresponding JEventSourceGenerator found, ask JApplication to quit, and return nullptr
    jerr << endl;
    jerr << "-----------------------------------------------------------------" << endl;
    jerr << " You specified event source type \"" << event_source_type << "\"" << endl;
    jerr << " be used to read the event sources but no such type exists." << endl;
    jerr << " Here is a list of available source types:" << endl;
    jerr << endl;
    for (auto sg : _eventSourceGenerators) jerr << "   " << sg->GetType() << endl;
    jerr << endl;
    jerr << "-----------------------------------------------------------------" << endl;
    mApplication->SetExitCode(-1);
    mApplication->Quit();
	return nullptr;
}

//---------------------------------
// GetNumEventsProcessed
//---------------------------------
std::size_t JEventSourceManager::GetNumEventsProcessed(void) const
{
	// Lock
	std::lock_guard<std::mutex> lg(mSourcesMutex);

	auto sNumEventGetter = [](std::size_t aTotal, JEventSource* aSource) -> std::size_t {return aTotal + aSource->GetNumEventsProcessed();};

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

	if(!_sources_unopened.empty())
		return false;

	auto sClosedChecker = [](JEventSource* aSource) -> bool {return aSource->IsExhausted();};
	return std::all_of(std::begin(_sources_active), std::end(_sources_active), sClosedChecker);
}
