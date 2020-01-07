//
//    File: JStatus.cc
// Created: Wed Dec 27 18:08:16 EST 2017
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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>
#include <dlfcn.h>
#include <string.h>

#include <set>
#include <map>

#include <JANA/Status/JStatus.h>
#include <JANA/JLogger.h>
#include <JANA/JEventSource.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Utils/JCpuInfo.h>

using namespace std;

#define MAX_FRAMES 256


JStatus *gJSTATUS = nullptr;
static map<pthread_t, JStatus::BACKTRACE_INFO_t> BACKTRACES;
//static map<pthread_t, int> CPU_NUMBER;
//static map<pthread_t, vector<void*> > BACKTRACES;
//static map<pthread_t, vector<char[256]> > BACKTRACE_SYMBOLS;
static uint32_t BACKTRACES_COMPLETED;
static mutex BT_MUTEX;
static bool FILTER_TRACES = true;

static JStatus::char_str char_str_empty = {""};
//static char char_str_empty[512] = {""};

//---------------------------------
// JStatus    (Constructor)
//---------------------------------
JStatus::JStatus()
{
	/// Construct a JStatus object. JStatus is a singleton and can only be 
	/// constructed via a call to the static "JStatus::Report()" method.

	// Get filename for the fifo. We use a generic name of /tmp/jana_status for the default
	// (as opposed to a unique name based on pid) so as not to fill the /tmp directory
	// with obsolete pipes. This means some care should be taken by the user to ensure
	// conflicts don't occur between multiple processes trying to use this feature
	// simultaneously.
	path = "/tmp/jana_status";
	japp->GetJParameterManager()->SetDefaultParameter(
		"JANA:STATUS_FNAME", 
		path, 
		"Filename of named pipe that can be used to get instantaneous status info. of running proccess.");

	jout << "Creating pipe named \"" << path << "\" for status info." << jendl;
	mkfifo( path.c_str(), 0666);
	
	gJSTATUS = this;
}

//---------------------------------
// ~JStatus    (Destructor)
//---------------------------------
JStatus::~JStatus()
{

}

//---------------------------------
// Report
//---------------------------------
void JStatus::Report(void){

	/// Static method used to generate and write the current status of the JANA process
	/// to the named pipe. On the first call, this will create a JStatus object and
	/// create the named pipe (see JANA::STATUS_FNAME for name). Subsequent calls
	/// will open the pipe, write to it, and close it. This is generally called when
	/// the process receives a USR1 signal. Typically, if a single JANA process is
	/// running on the node, then one can issue a "killall <progname> -USR1" and then
	/// do a "cat /tmp/jana_status" to see the report.

	// Create a JStatus object if one does not already exist
	if( gJSTATUS == nullptr ) new JStatus();

	// Generate a report
	std::stringstream ss;
	gJSTATUS->GenerateReport( ss );
std::cerr << ss.str(); //TODO: FIX ME: This is a hack because I couldn't seem to get an output file
	gJSTATUS->SendReport( ss );
}

//---------------------------------
// GenerateReport
//---------------------------------
void JStatus::GenerateReport(std::stringstream &ss)
{
	auto t = time(NULL);

	vector<JEventProcessor*> processors;
	vector<JEventSource*> sources;
	vector<JEventSourceGenerator*> source_generators;
	vector<JFactoryGenerator*> factory_generators;
/*	std::vector<JThreadManager::JEventSourceInfo*> active_source_infos;
	std::vector<JThreadManager::JEventSourceInfo*> retired_source_infos;
	vector<JThread*> threads;
*/
/*
	japp->GetJEventProcessors(processors);
	japp->GetJEventSourceManager()->GetActiveJEventSources(sources); //ignores exhausted sources!!
	japp->GetJEventSourceManager()->GetJEventSourceGenerators(source_generators);
	japp->GetJFactoryGenerators(factory_generators);
    japp->GetJThreadManager()->GetActiveSourceInfos(active_source_infos);
	japp->GetJThreadManager()->GetRetiredSourceInfos(retired_source_infos); //assumes one didn't retire in between calls!
	japp->GetJThreadManager()->GetJThreads(threads);
*/

	std::size_t sNumQueues = 0;
/*	for(auto& sSourceInfo : active_source_infos)
		sNumQueues += sSourceInfo->mQueueSet->GetNumQueues();
	for(auto& sSourceInfo : retired_source_infos)
		sNumQueues += sSourceInfo->mQueueSet->GetNumQueues();
*/

	ss << "------ JANA STATUS REPORT -------" << endl;
	ss << "generated: " << ctime(&t);
	ss << endl;
//	ss << "      Nthreads/Ncores: " << japp->GetJThreadManager()->GetNJThreads() << " / " << JCpuInfo::GetNumCpus() << endl;
	ss << "    Nevents processed: " << japp->GetNeventsProcessed() << endl;
	ss << "          Nprocessors: " << processors.size() << endl;
	ss << "             Nsources: " << sources.size() << endl;
	ss << "    Nsourcegenerators: " << source_generators.size() << endl;
	ss << "   Nfactorygenerators: " << factory_generators.size() << endl;
	ss << "              Nqueues: " << sNumQueues << endl;
	ss << endl;

/*
	for(auto& sSourceInfo : active_source_infos)
	{
		auto sEventSource = sSourceInfo->mEventSource;
		ss << "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = " << endl;
		ss << "Event Source: " << sEventSource->GetName() << " [" << sEventSource->GetType() << "]" << endl;
		ss << "                      Description: " << sEventSource->GetVDescription() << endl;
		ss << "                      IsExhausted: " << sEventSource->IsExhausted() << endl;
		ss << "             Num events processed: " << sEventSource->GetNumEventsProcessed() << endl;
		ss << "           Num outstanding events: " << sEventSource->GetNumOutstandingEvents() << endl;
		ss << "   Num outstanding barrier events: " << sEventSource->GetNumOutstandingBarrierEvents() << endl;
		ss << endl;
	
		auto sQueueSet = sSourceInfo->mQueueSet;
		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sQueuesByType;
		sQueueSet->GetQueues(sQueuesByType);
		for(auto& sQueueTypePair : sQueuesByType)
		{
			auto& sQueues = sQueueTypePair.second;
			for(auto sQueue : sQueues)
			{
				ss << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " << endl;
				ss << "  active source, queue: " << sSourceInfo->mEventSource->GetName() << ", " << sQueue->GetName() << ": " << endl;
				ss << "                       Ntasks: " << sQueue->GetNumTasks() <<endl;
				ss << "             Ntasks processed: " << sQueue->GetNumTasksProcessed() << endl;
				ss << "         Max allowed in queue: " << sQueue->GetMaxTasks() << endl;
				ss << endl;
			}
		}
	}

	for(auto& sSourceInfo : retired_source_infos)
	{
		auto sQueueSet = sSourceInfo->mQueueSet;
		std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sQueuesByType;
		sQueueSet->GetQueues(sQueuesByType);
		for(auto& sQueueTypePair : sQueuesByType)
		{
			auto& sQueues = sQueueTypePair.second;
			for(auto sQueue : sQueues)
			{
				ss << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " << endl;
				ss << " retired source, queue: " << sSourceInfo->mEventSource->GetName() << ", " << sQueue->GetName() << ": " << endl;
				ss << "                       Ntasks: " << sQueue->GetNumTasks() <<endl;
				ss << "             Ntasks processed: " << sQueue->GetNumTasksProcessed() << endl;
				ss << "         Max allowed in queue: " << sQueue->GetMaxTasks() << endl;
				ss << endl;
			}
		}
	}
*/
	// The only way to get a stack trace for a thread is from within the thread
	// itself. To do this, we must signal every thread to interrupt it so it
	// can record its current stack trace.
	//
	// The C++ language does not provide a way to signal individual or even
	// all threads. There is a way to do it with pthreads though and both
	// Linux and Max OS X use pthreads as the underlying thread package.
	// To try and make this as portable as possible, we check if the 
	// native_type_handle is a pthread_t. Note that not even that is guaranteed
	// by the C++ standard to be defined.

	ss << endl;
	if( typeid(thread::native_handle_type) == typeid(pthread_t) ){
		ss << "underlying thread model: pthreads" << endl;
	
		// Get list of pthreads, including this one (which is hopefully the main thread)
		map<std::thread::id, pthread_t> pthreads;
		auto sThisThreadID = std::this_thread::get_id();
/*
		for(auto t : threads)
		{
			auto sID = t->GetThread()->get_id();
			if(sID != sThisThreadID)
				pthreads.emplace(sID, t->GetThread()->native_handle());
		}
*/
		pthreads.emplace(sThisThreadID, pthread_self());
		
		// Pre-allocate memory to hold the traces for all threads.
		// We must do this here because the signals we send next
		// may interrupt a malloc call and if we try calling malloc
		// while recording the trace, we'll become deadlocked.
		BACKTRACES.clear();
		BACKTRACES_COMPLETED = 0;
		for(auto& pthr_pair : pthreads) {
			auto& pthr = pthr_pair.second;
			auto &btinfo = BACKTRACES[pthr];
			btinfo.cpuid = -1;
			btinfo.bt.reserve(MAX_FRAMES);
			btinfo.bt_symbols.reserve(MAX_FRAMES);
			btinfo.bt_fnames.reserve(MAX_FRAMES);
		}
		
		// Loop over all threads, signaling them to generate a backtrace
		// Except the current thread!  We'll call it directly here.
		// E.g. if a thread segfault's then gets the SIGUSR2 signal, I think the full stack trace is lost??
		for(auto& pthr_pair : pthreads)
		{
			if(pthr_pair.first == sThisThreadID)
				continue;
			auto& pthr = pthr_pair.second;
			pthread_kill( pthr, SIGUSR2 );
		}
		
		//Record the back trace for THIS thread
		JStatus::RecordBackTrace();

		// Wait for all other threads to finish their backtraces. Limit how long we'll wait
		for(int i=0; i<1000; i++){

			this_thread::sleep_for( chrono::microseconds(1000) );
			
			lock_guard<mutex> lg(BT_MUTEX);
			if( BACKTRACES_COMPLETED == pthreads.size() ) break;
		}
		
		// Loop over threads, printing backtrace results
		lock_guard<mutex> lg(BT_MUTEX);
		for(auto& pthr_pair : pthreads) {
			auto& pthr = pthr_pair.second;
			ss << endl;
			ss << "native handle: " << hex << pthr << dec;
			if( pthr==pthread_self() ) ss << "  (probably main thread)";
			ss << endl;
			if( BACKTRACES.count(pthr)==1 ){
				ss << BackTraceToString(BACKTRACES[pthr]);
			}else{
				ss << "  < backtrace unavailable >" << endl;
			}
		}
	}else{
		ss << "underlying thread model: unknown" << endl;
	}

	// Release any memory allocated for backtraces
	BACKTRACES.clear();
}

//---------------------------------
// SendReport
//---------------------------------
void JStatus::SendReport(std::stringstream &ss)
{
	// Open pipe and write report if successful. Close pipe when done.
	int fd = open( path.c_str(), O_WRONLY );
	if( fd>=0 ){
		write(fd, ss.str().c_str(), ss.str().length()+1);
		close(fd);
	}else{
		jerr << "Unable to open named pipe \"" << path << "\" for writing status!" << jendl;
	}
}

//---------------------------------
// RecordBackTrace
//---------------------------------
void JStatus::RecordBackTrace(void)
{
	/// Record the backtrace for the current thread in the 
	/// BACKTRACES global. 

	// get trace
	void *trace[MAX_FRAMES];
	int trace_size = backtrace(trace,MAX_FRAMES);

	// Lock mutex while writing to globals
	lock_guard<mutex> lg(BT_MUTEX);

	// Write trace info to pre-allocated containers
	auto &btinfo = BACKTRACES[pthread_self()];
	btinfo.cpuid = JCpuInfo::GetCpuID();
	btinfo.bt_symbols.resize(trace_size, char_str_empty);
	btinfo.bt_fnames.resize(trace_size, char_str_empty);
	for(int i=0; i<trace_size; i++){
		btinfo.bt.push_back(trace[i]);
		
		// n.b. we do not use backtrace_symbols here because
		// it allocates memory which could cause a deadlock
		// if we interrupted a malloc call to do this trace.
		// Linking with -Wl,-E option on Linux (and without
		// any option on Mac OS X) makes the symbols available
		// to dladdr in the dynamic symbols table.
		Dl_info dlinfo;
		if(dladdr(trace[i], &dlinfo)){
			strcpy(btinfo.bt_symbols[i].x, dlinfo.dli_sname ? dlinfo.dli_sname:"<N/A>*");
			strcpy(btinfo.bt_fnames[i].x , dlinfo.dli_fname ? dlinfo.dli_fname:"<N/A>*");
		}else{
			strcpy(btinfo.bt_symbols[i].x, "<N/A>");
			strcpy(btinfo.bt_fnames[i].x , "<N/A>");
		}
	}

	// Increment global counter indicating we're done.
	// (n.b. mutex is still locked here)
	BACKTRACES_COMPLETED++;
}

//---------------------------------
// BackTraceToString
//---------------------------------
string JStatus::BackTraceToString(BACKTRACE_INFO_t &btinfo)
{
	/// Convert the given trace to a multi-line string.

	// Loop over symbols in backtrace
	stringstream ss;
	ss << "CPU: " << btinfo.cpuid << endl;
	for(uint32_t i=0; i<btinfo.bt_symbols.size(); i++) {
	
		// Try demangling the name of the symbol
		string name = btinfo.bt_symbols[i].x;
		if( name.find("<N/A>") != string::npos ) name = btinfo.bt_fnames[i].x;
		size_t dlen = 1023;
		char dname[1024];
		int status=0;
		abi::__cxa_demangle(name.c_str(), dname, &dlen, &status);
		string demangled_name(status ? "":dname);
		if(status==0) name = demangled_name;

/*
		//NO!!!! THIS IS NEEDED!!!!
		// Truncate really long names since they are usually indiscernable
		if( name.length()>40 ) {
			name.erase(40);
			name += " ...";
		}
*/
		// Ignore frames where the name seems un-useful
		if( FILTER_TRACES ){
			// OS X
			if( name.find("?") != string::npos ) continue;
			if( name.find("JStatus") != string::npos ) continue;
			if( name == "0x0") continue;
			if( name == "_sigtramp" ) continue;
			if( name == "_pthread_body" ) continue;
			if( name == "thread_start" ) continue;
		}

		// Add demangled name or full message to output
		ss << string(i+5, ' ') + "`- " << name << " (" << hex << btinfo.bt[i] << dec << ")" << endl;
	}
	ss << ends;

	return ss.str();
}


