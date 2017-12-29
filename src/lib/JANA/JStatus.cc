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

#include "JParameterManager.h"
#include "JStatus.h"
#include "JThread.h"
#include "JQueue.h"

#include <time.h>
#include <pthread.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>

#include <set>
#include <map>
using namespace std;

JStatus *gJSTATUS = nullptr;
static map<pthread_t, string> BACKTRACES;
static mutex BT_MUTEX;
static bool FILTER_TRACES = true;

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
	gPARMS->SetDefaultParameter("JANA:STATUS_FNAME", path, "Filename of named pipe that can be used to get instantaneous status info. of running proccess.");

	jout << "Creating pipe named \"" << path << "\" for status info." << endl;
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
// GenerateReport
//---------------------------------
void JStatus::GenerateReport(std::stringstream &ss)
{
	auto t = time(NULL);

	vector<JEventProcessor*> processors;
	vector<JEventSource*> sources;
	vector<JEventSourceGenerator*> source_generators;
	vector<JFactoryGenerator*> factory_generators;
	vector<JQueue*> queues;
	vector<JThread*> threads;

	japp->GetJEventProcessors(processors);
	japp->GetJEventSources(sources);
	japp->GetJEventSourceGenerators(source_generators);
	japp->GetJFactoryGenerators(factory_generators);
	japp->GetJQueues(queues);
	japp->GetJThreads(threads);

	ss << "------ JANA STATUS REPORT -------" << endl;
	ss << "generated: " << ctime(&t);
	ss << endl;
	ss << "      Nthreads/Ncores: " << japp->GetNJThreads() << " / " << japp->GetNcores() << endl;
	ss << "    Nevents processed: " << japp->GetNeventsProcessed() << endl;
	ss << "          Nprocessors: " << processors.size() << endl;
	ss << "             Nsources: " << sources.size() << endl;
	ss << "    Nsourcegenerators: " << source_generators.size() << endl;
	ss << "   Nfactorygenerators: " << factory_generators.size() << endl;
	ss << "              Nqueues: " << queues.size() << endl;
	ss << endl;
	
	for(auto q : queues){
		ss << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " << endl;
		ss << "  queue " << q->GetName() << ":" << endl;
		ss << "                       Nevents: " << q->GetNumEvents() <<endl;
		ss << "             Nevents processed: " << q->GetNumEventsProcessed() << endl;
		ss << "          Max allowed in queue: " << q->GetMaxEvents() << endl;
		ss << "             ConvertFrom types: ";
		for( auto s : q->GetConvertFromTypes() ) ss << s << ", ";
		ss << endl;
	}
	
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
	
		// Clear any existing backtraces
		BACKTRACES.clear();
				
		// Get list of pthreads, including this one which is hopefully the main thread
		set<pthread_t> pthreads;
		pthreads.insert( pthread_self() );
		for(auto t : threads) pthreads.insert( t->GetThread()->native_handle() );
		
		// Loop over all threads, signaling them to generate a backtrace
		jout << "Number of pthreads: " << pthreads.size() << endl;
		for(auto pthr : pthreads) pthread_kill( pthr, SIGUSR2 );
		
		// Wait for all threads to finish their backtraces. Limit how long we'll wait
		for(int i=0; i<1000; i++){

			this_thread::sleep_for( chrono::microseconds(1000) );
			
			lock_guard<mutex> lg(BT_MUTEX);
			if( BACKTRACES.size() == pthreads.size() ) break;
		}
		
		// Loop over threads, printing backtrace results
		lock_guard<mutex> lg(BT_MUTEX);
		for(auto pthr : pthreads){
			ss << endl;
			ss << "native handle: " << pthr;
			if( pthr==pthread_self() ) ss << "  (probably main thread)";
			ss << endl;
			if( BACKTRACES.count(pthr)==1 ){
				ss << BACKTRACES[pthr];
			}else{
				ss << "  < backtrace unavailable >" << endl;
			}
		}
	}else{
		ss << "underlying thread model: unknown" << endl;
	}

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
		jerr << "Unable to open named pipe \"" << path << "\" for writing status!" << endl;
	}
}

//---------------------------------
// RecordBackTrace
//---------------------------------
void JStatus::RecordBackTrace(void)
{
	lock_guard<mutex> lg(BT_MUTEX);

	size_t dlen = 1023;
	char dname[1024];
	void *trace[1024];
	int status=0;

	// get trace messages
	int trace_size = backtrace(trace,1024);
	if(trace_size>1024) trace_size=1024;
	char **messages = backtrace_symbols(trace, trace_size);

	// de-mangle and create string
	stringstream ss;
	int max_frames = 1024;
	int start_frame = trace_size - max_frames;
	if(start_frame < 0) start_frame = 0;
	for(int i=start_frame; i<trace_size; ++i) {

		if(!messages[i])continue;
		string message(messages[i]);

		// Find mangled name.
		//
		// It seems on Linux and possibly older versions of Mac OS X,
		// the messages from backtrace_symbols have a format:
		//
		// ./backtrace_test(_Z5alphav+0x9) [0x400b0d]
		//
		// while on more recent OS X versions it has a format:
		//
		// 3   backtrace_test                      0x000000010dc2fbc9 _Z5alphav + 9

		//
		// First, we try pulling out the name assuming Linux style. If that
		// doesn't work, we try OS X style
		size_t pos_start = message.find_first_of('(');
		size_t pos_end = string::npos;
		if(pos_start != string::npos) pos_end = message.find_first_of('+', ++pos_start);
		if(pos_end != string::npos){
			// Linux style
			// (nothing to do)
		}else{
			// OS X style
			pos_end = message.find_last_of('+');
			if(pos_end != string::npos && pos_end>2){
				pos_start = message.find_last_of(' ', pos_end-2);
				if(pos_start != string::npos){
					pos_start++; // advance to first character of mangled name
					pos_end--; // backup to space just after mangled name
				}
			}
		}
		// Peel out mangled name into a separate string
		string mangled_name = "";
		if(pos_start!=string::npos && pos_end!=string::npos){
			mangled_name = message.substr(pos_start, pos_end-pos_start);
		}

		// Demangle name
		abi::__cxa_demangle(mangled_name.c_str(),dname,&dlen,&status);
		string demangled_name(status ? "":dname);
		
		string name = message;
		if(demangled_name.length()>0){
			name = demangled_name + "**";
		}else if(mangled_name.length()>0){
			name = mangled_name + "*";
		}
		
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
		ss << string(i+20, ' ') + "`- " << name << endl;
	}
	ss << ends;

	free(messages);

	BACKTRACES[pthread_self()] = ss.str();
}


