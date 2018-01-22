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

#include "JEventSourceManager.h"

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
// GetNextEvent
//---------------------------------
uint32_t JEventSourceManager::GetNextEvent(void)
{
	/// Read in a JEvent from one of the active sources. If there are
	/// no active sources, open the next source. If all sources have
	/// been exhausted, then the JEventSourceManager will be flagged to quit
	/// and a JException will be thrown.

	// This is designed to allow multiple threads to read from multiple
	// input streams simultaneously. It uses atomics to avoid mutex locking
	// where possible. This does complicate things a bit so here goes an
	// explanation:
	//
	// 1. The _sources_active container is sized to hold the maximum number
	// of simultaneous event streams. Each element will contain either a NULL
	// pointer or a pointer to a valid JEventSource object.
	//
	// 2. This will first look for a non-NULL value in _active_sources. If
	// one is found, it will try and claim exclusive use via its _in_use
	// atomic member. If it obtains that, then it will either read an
	// event from it, or, if there are no more events, set the slot in
	// _actve_sources to NULL indicating it can be filled by another source.
	// The exhausted pointer is then added to the _sources_exhausted container.
	//
	// 3. If exclusive use of a source cannot be obtained and there is at
	// least one NULL pointer in _active_sources, then OpenNext() will be
	// called. That will lock a mutex while creating a new JEventSource
	// and placing it in the _active_sources queue.
	//
	// Mutexes are only used when:
	//
	// A. creating a new JEventSource and writing it into _active_sources
	//
	// B. writing an exhausted JEventSource into _sources_exhausted
	//
	// Both of those should happen infrequently. Most calls to this should
	// result in either an event being read in or nothing at all happening.
	// Most of the time only accesses to a few atomic variables will occur.

	bool has_null_slot = false;
	for(uint32_t islot = 0; islot < _sources_active.size(); islot++)
	{
		auto src = _sources_active[islot];
		if( src == nullptr )
		{
			has_null_slot = true;
			continue;
		}

		bool tmp = false;
		if(!src->_in_use.compare_exchange_weak(tmp, true) )
			continue;

		// We now have exclusive use of src
		if( src->IsDone() ){
			// No more events in this source. Retire it.
			_sources_active[islot] = nullptr;
			std::lock_guard<std::mutex> lguard(_sources_exhausted_mutex);
			_sources_exhausted.push_back( src );
		} else {
			try{
				// Read event from this source
				switch( src->GetEvent() ){
					case JEventSource::kSUCCESS:
					case JEventSource::kNO_MORE_EVENTS:
						// Not throwing an exception here will cause the calling
						// JThread to immediately cycle back to start of Loop
						// without sleeping.
						break;
					case JEventSource::kTRY_AGAIN:
						// This will cause the calling JThread to sleep briefly
						src->_in_use.store( false );
						throw JException("source stalled. try again.");
						break;
					default:
						src->_in_use.store( false );
						throw JException("Unknown response from event source.");
						break;

				}
			}catch(JException &e){
				// There was a problem reading from this source.
				// Make sure it is flagged as done so it will be
				// cleaned up on a subsequent call.
				jerr << e.GetMessage() << endl;
				src->SetDone(true);
			}
		}

		// Free the _in_use flag
		src->_in_use.store( false );

		// At this point we have either read in an event or changed
		// the state of a source.
		return JApplication::kSUCCESS;
	}

	// If we get here then we were not able to get exclusive use of an existing
	// source object. If there are no empty slots then we are waiting on other
	// threads to read in events. Throw an exception to cause the calling JThread
	// to sleep breifly before trying again.
	if( !has_null_slot ) return JApplication::kTRY_AGAIN;
	//if( !has_null_slot ) throw JException("thread stalled waiting for event");

	// There was and possibly still is at least one empty slot for an event source.
	// Try opening the next source.
	OpenNext();

	// Return kSUCCESS to tell calling thread to loop back
	// immediately without sleeping since there may now be
	// something for it to do.
	return JApplication::kSUCCESS;
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
		OpenSource(si, source_name);
	}
}

//---------------------------------
// OpenSource
//---------------------------------
void JEventSourceManager::OpenSource(std::size_t islot, const std::string& source_name)
{
	// Check if the user has forced a specific type of event source
	// be used via the EVENT_SOURCE_TYPE config. parameter. If so,
	// search for that source and use it. Otherwise, throw an exception.
	auto gen = GetUserEventSourceGenerator();

	if(gen == nullptr)
		gen = GetEventSourceGenerator(source_name);

	// Try openng the source using the chosen generator
	JEventSource *new_source = nullptr;
	if(gen != nullptr){
		jout << "Opening source \"" << source_name << "\" of type: "<< gen->Description() << endl;
		new_source = gen->MakeJEventSource(source_name);
	}

	if(new_source){

		// Copy pointer into the empty active pointers slot
		_sources_active[islot] = new_source;

	}else{

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
	}
}

//---------------------------------
// OpenNext
//---------------------------------
void JEventSourceManager::OpenNext(void)
{
	/// Try opening the next source in the list. This will return immediately
	/// if all sources have already been opened.

	// Lock mutex while we look for a slot with a nullptr
	std::lock_guard<std::mutex> lg(_sources_open_mutex);

	if( _source_names_unopened.empty() ) return;

	// Try and find a slot with a nullptr that we can grab exclusive use of.
	// Note that slots that do not contain a value of nullptr may be overwritten
	// to containe nullptr in GetNextEvent(). This is the only place where a
	// nullptr can be overwritten with something else though.

	uint32_t islot = 0;
	for(auto src : _sources_active){
		if( src == nullptr ) break;
		islot++;
	}
	if( islot >= _sources_active.size() ) return;

	// Get name of next source to open
	std::string source_name = _source_names_unopened.front();
	_source_names_unopened.pop_front();

	OpenSource(islot, source_name);
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
