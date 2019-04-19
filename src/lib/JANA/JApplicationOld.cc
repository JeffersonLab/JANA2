//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
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
//
// Authors: Nathan Brei
//



#include <unordered_set>

#include <JANA/JApplicationOld.h>

#include <JANA/JThreadManager.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceManager.h>

JApplicationOld::JApplicationOld(JParameterManager* params, std::vector<std::string>* event_sources)
        : JApplication(params, event_sources) {

    jout << "Instantiating JApplicationOld" << std::endl;

    _threadManager = new JThreadManager(this);
}

JApplicationOld::~JApplicationOld() {
	if (_threadManager != nullptr) delete _threadManager;
}

void JApplicationOld::Initialize() {
	/// Initialize the application in preparation for data processing.
	/// This is called by the Run method so users will usually not
	/// need to call this directly.

	if (_initialized) return;

	// Set number of threads
	_nthreads = JCpuInfo::GetNumCpus();
	_pmanager->SetDefaultParameter("NTHREADS", _nthreads, "The total number of worker threads");

	// Set task pool size
	int task_pool_size = 200;
	int task_pool_debuglevel = 0;
	_pmanager->SetDefaultParameter("JANA:TASK_POOL_SIZE", task_pool_size, "Task pool size");
	_pmanager->SetDefaultParameter("JANA:TASK_POOL_DEBUGLEVEL", task_pool_debuglevel, "Task pool debug level");
	mVoidTaskPool.Set_ControlParams(task_pool_size, task_pool_debuglevel);

	// Attach all plugins
	AttachPlugins();

	// Create all event sources
	_eventSourceManager->CreateSources();

	// Get factory generators from event sources
	std::deque<JEventSource*> sEventSources;
	_eventSourceManager->GetUnopenedJEventSources(sEventSources);
	std::unordered_set<std::type_index> sSourceTypes;
	for(auto sSource : sEventSources)
	{
		auto sTypeIndex = sSource->GetDerivedType();
		if(sSourceTypes.find(sTypeIndex) != std::end(sSourceTypes))
			continue; //same type as before: duplicate factories!

		auto sGenerator = sSource->GetFactoryGenerator();
		if(sGenerator != nullptr)
			_factoryGenerators.push_back(sGenerator);
	}

	//Prepare for running: Open event sources and prepare task queues for them
	_eventSourceManager->OpenInitSources();
	_threadManager->PrepareQueues();

}


void JApplicationOld::Run() {

	// Setup all queues and attach plugins
	Initialize();

	// If something went wrong in Initialize() then we may be quitting early.
	if(_quitting) return;

	// Create all remaining threads (one may have been created in Init)
	LOG_INFO(_logger) << "Creating " << _nthreads << " processing threads ..." << LOG_END;
	_threadManager->CreateThreads(_nthreads);

	// Optionally set thread affinity
	try{
		int affinity_algorithm = 0;
		_pmanager->SetDefaultParameter("AFFINITY", affinity_algorithm, "Set the thread affinity algorithm. 0=none, 1=sequential, 2=core fill");
		_threadManager->SetThreadAffinity( affinity_algorithm );
	}catch(...){
		LOG_ERROR(_logger) << "Unknown exception in JApplication::Run attempting to set thread affinity" << LOG_END;
	}

	// Print summary of all config parameters (if any aren't default)
	GetJParameterManager()->PrintParameters(false);

	// Start all threads running
	jout << "Start processing ..." << std::endl;
	mRunStartTime = std::chrono::high_resolution_clock::now();
	_threadManager->RunThreads();

	// Monitor status of all threads
	while( !_quitting ){

		// If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
		// This flag is used by the integrated rate calculator
		// The JThreadManager is in charge of telling all the threads to end
		if(!_draining_queues)
			_draining_queues = _eventSourceManager->AreAllFilesClosed();

		// Check if all threads have finished
		if(_threadManager->AreAllThreadsEnded())
		{
			std::cout << "All threads have ended.\n";
			break;
		}

		// Sleep a few cycles
		std::this_thread::sleep_for( std::chrono::milliseconds(500) );

		// Print status
		if( _ticker_on ) PrintStatus();
	}

	// Join all threads
	jout << "Event processing ended. " << std::endl;
	if (!_skip_join) {
		jout << "Merging threads ..." << std::endl;
		_threadManager->JoinThreads();
	}

	// Delete event processors
	for(auto sProcessor : _eventProcessors){
		sProcessor->Finish(); // (this may not be necessary since it is always next to destructor)
		delete sProcessor;
	}
	_eventProcessors.clear();

	// Report Final numbers
	PrintFinalReport();
}

void JApplicationOld::Scale(int nthreads) {
    _nthreads = nthreads;
	_threadManager->SetNJThreads(nthreads);
}

void JApplicationOld::Quit(bool skip_join) {
	_skip_join = skip_join;
	_threadManager->EndThreads();
	_quitting = true;
}

void JApplicationOld::Stop(bool wait_until_idle) {
	/// Tell all JThread objects to go into the idle state after completing
	/// their current task. If wait_until_idle is true, this will block until
	/// all threads are in the idle state. If false (the default), it will return
	/// immediately after telling all threads to go into the idle state
	_threadManager->StopThreads(wait_until_idle);
}

void JApplicationOld::Resume() {
	/// Tell all JThread objects to go into the running state (if not already there).
	_threadManager->RunThreads();
}

JThreadManager* JApplicationOld::GetJThreadManager() const {
    return _threadManager;
}

std::shared_ptr<JTask<void>> JApplicationOld::GetVoidTask() {
	return mVoidTaskPool.Get_SharedResource();
}

void JApplicationOld::UpdateResourceLimits() {
    /// Used internally by JANA to adjust the maximum size of resource
    /// pools after changing the number of threads.

    // OK, this is tricky. The max size of the JFactorySet resource pool should
    // be at least as big as how many threads we have. Factory sets may also be
    // attached to JEvent objects in queues that are not currently being acted
    // on by a thread so we actually need the maximum to be larger if we wish to
    // prevent constant allocation/deallocation of factory sets. If the factory
    // sets are large, this will cost more memory, but should save on CPU from
    // allocating less often and not having to call the constructors repeatedly.
    // The exact maximum is hard to determine here. We set it to twice the number
    // of threads which should be sufficient. The user should be given control to
    // adjust this themselves in the future, but or now, this should be OK.
	auto nthreads = _threadManager->GetNJThreads();
	mFactorySetPool.Set_ControlParams( nthreads*2, 10 );
}

void JApplicationOld::PrintFinalReport() {

	//Get queues
	std::vector<JThreadManager::JEventSourceInfo*> sRetiredQueues;
	_threadManager->GetRetiredSourceInfos(sRetiredQueues);
	std::vector<JThreadManager::JEventSourceInfo*> sActiveQueues;
	_threadManager->GetActiveSourceInfos(sActiveQueues);
	auto sAllQueues = sRetiredQueues;
	sAllQueues.insert(sAllQueues.end(), sActiveQueues.begin(), sActiveQueues.end());


	// Get longest JQueue name
	uint32_t sSourceMaxNameLength = 0, sQueueMaxNameLength = 0;
	for(auto& sSourceInfo : sAllQueues)
	{
		auto sSource = sSourceInfo->mEventSource;
		auto sSourceLength = sSource->GetName().size();
		if(sSourceLength > sSourceMaxNameLength)
			sSourceMaxNameLength = sSourceLength;

		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
		sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
		for(auto& sTypePair : sSourceQueues)
		{
			for(auto sQueue : sTypePair.second)
			{
				auto sLength = sQueue->GetName().size();
				if(sLength > sQueueMaxNameLength)
					sQueueMaxNameLength = sLength;
			}
		}
	}
	sSourceMaxNameLength += 2;
	if(sSourceMaxNameLength < 8) sSourceMaxNameLength = 8;
	sQueueMaxNameLength += 2;
	if(sQueueMaxNameLength < 7) sQueueMaxNameLength = 7;

	jout << std::endl;
	jout << "Final Report" << std::endl;
	jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
	jout << "Source" << std::string(sSourceMaxNameLength - 6, ' ') << "   Nevents  " << "Queue" << std::string(sQueueMaxNameLength - 5, ' ') << "NTasks" << std::endl;
	jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
	std::size_t sSrcIdx = 0;
	for(auto& sSourceInfo : sAllQueues)
	{
		// Flag to prevent source name and number of events from
		// printing more than once.
		bool sSourceNamePrinted = false;

		// Place "*" next to names of active sources
		string sFlag;
		if( sSrcIdx++ >= sRetiredQueues.size() ) sFlag="*";

		auto sSource = sSourceInfo->mEventSource;
		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
		sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
		for(auto& sTypePair : sSourceQueues)
		{
			for(auto sQueue : sTypePair.second)
			{
				string sSourceName = sSource->GetName()+sFlag;

				if( sSourceNamePrinted ){
					jout << string(sSourceMaxNameLength + 12, ' ');
				}else{
					sSourceNamePrinted = true;
					jout << sSourceName << string(sSourceMaxNameLength - sSourceName.size(), ' ')
						 << std::setw(10) << sSource->GetNumEventsProcessed() << "  ";
				}

				jout << sQueue->GetName() << string(sQueueMaxNameLength - sQueue->GetName().size(), ' ')
					 << sQueue->GetNumTasksProcessed() << std::endl;
			}
		}
	}
	if( !sActiveQueues.empty() ){
		jout << std::endl;
		jout << "(*) indicates sources that were still active" << std::endl;
	}

	jout << std::endl;
	jout << "Total events processed: " << GetNeventsProcessed() << " (~ " << Val2StringWithPrefix( GetNeventsProcessed() ) << "evt)" << std::endl;
	jout << "Integrated Rate: " << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz" << std::endl;
	jout << std::endl;

	// Optionally print more info if user requested it:
	bool print_extended_report = false;
	_pmanager->SetDefaultParameter("JANA:EXTENDED_REPORT", print_extended_report);
	if( print_extended_report ){
		jout << std::endl;
		jout << "Extended Report" << std::endl;
		jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
		jout << "               Num. plugins: " << _plugins.size() <<std::endl;
		jout << "          Num. plugin paths: " << _plugin_paths.size() <<std::endl;
		jout << "    Num. factory generators: " << _factoryGenerators.size() <<std::endl;
		jout << "Num. calibration generators: " << _calibrationGenerators.size() <<std::endl;
		jout << "      Num. event processors: " << mNumProcessorsAdded <<std::endl;
		jout << "          Num. factory sets: " << mFactorySetPool.Get_PoolSize() << " (max. " << mFactorySetPool.Get_MaxPoolSize() << ")" << std::endl;
		jout << "       Num. config. params.: " << _pmanager->GetNumParameters() <<std::endl;
		jout << "               Num. threads: " << _threadManager->GetNJThreads() <<std::endl;
	}
}
















