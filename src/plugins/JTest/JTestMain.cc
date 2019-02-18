//
//    File: JTestMain.cc
// Created: Fri Dec  7 08:42:59 EST 2018
// Creator: davidl (on Linux jana2.jlab.org 3.10.0-957.el7.x86_64 x86_64)
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
// Description
//
// This is used as an overall orchestrator for the various testing modes of the JTest
// plugin. This allows it to do more advanced testing by setting the JTEST:MODE and
// other config. parameters. This could have all been embedded in the JEventProcessor
// class, but I decided to put it here to make things a little cleaner.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
using std::cout;
using std::cerr;
using std::endl;

#include <JApplication.h>
#include <JEventSourceGeneratorT.h>

#include "JTestMain.h"
#include "JEventSource_jana_test.h"
#include "JEventProcessor_jana_test.h"
#include "JFactoryGenerator_jana_test.h"

extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->Add(new JEventSourceGeneratorT<JEventSource_jana_test>());
	app->Add(new JFactoryGenerator_jana_test());
	app->Add(new JEventProcessor_jana_test(app));

	new JTestMain(app);
}
} // "C"

//---------------------------------
// JTestMain    (Constructor)
//---------------------------------
JTestMain::JTestMain(JApplication *app)
{
	mApp = app;
	mLogger = app->GetJLogger();

	string kThreadSet;
	mOutputDirName="JANA_Test_Results";

	auto params = app->GetJParameterManager();

	params->SetDefaultParameter(
		"JTEST:MODE", 
		mMode, 
		"JTest plugin Testing mode. 0=basic, 1=scaling");

	params->SetDefaultParameter(
		"JTEST:NSAMPLES", 
		mNsamples, 
		"JTest plugin number of samples to take for each test");

	uint32_t kMinThreads=1;
	params->SetDefaultParameter(
		"JTEST:MINTHREADS", 
		kMinThreads, 
		"JTest plugin minimum number of threads to test");

	uint32_t kMaxThreads = mApp->GetJThreadManager()->GetNcores();
	params->SetDefaultParameter(
		"JTEST:MAXTHREADS", 
		kMaxThreads, 
		"JTest plugin maximum number of threads to test");

	uint32_t kThreadStep=1;
	params->SetDefaultParameter(
		"JTEST:THREADSTEP",
		kThreadStep,
		"JTest plugin number of threads step size");

	params->SetDefaultParameter(
		"JTEST:RESULTSDIR", 
		mOutputDirName, 
		"JTest output directory name for sampling test results");

	// insert continuous range of NThreads to test
	if (mMode == 1) {
		for(uint32_t nthreads=kMinThreads; nthreads<=kMaxThreads; nthreads+=kThreadStep) mThreadSet.insert(nthreads);
	}

	// If no source has been specified, then add a dummy source
	if( app->GetJEventSourceManager()->GetSourceNames().empty() ) app->GetJEventSourceManager()->AddEventSource("dummy");

	switch( mMode ){
		case MODE_BASIC:
			break;
		case MODE_SCALING:
			params->SetParameter("NEVENTS", 0);
			mThread = new std::thread(&JTestMain::TestThread, this);
			break;
	}
}

//---------------------------------
// ~JTestMain    (Destructor)
//---------------------------------
JTestMain::~JTestMain()
{
	// Clean up thread if it was launched
	if(mThread != nullptr){
		mQuit = true;
		mThread->join();
		delete mThread;
		mThread = nullptr;
	}
}

//---------------------------------
// TestThread
//
// This method will be called as a separate thread to orchestrate
// the test depending on the testing mode.
//---------------------------------
void JTestMain::TestThread(void)
{
	//- - - - - - - - - - - - - - - - - - - - - - - -
	// Scaling test

	auto tm = mApp->GetJThreadManager();

	// Turn ticker off so we can better control the screen
	mApp->SetTicker( false );

	// Wait for events to start flowing indicating the source is primed
	for(int i=0; i<60; i++){
		cout << "Waiting for event source to start producing ... rate: " << mApp->GetInstantaneousRate() << endl;
		std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		auto rate = mApp->GetInstantaneousRate();
		if( rate > 10.0 ) {
			cout << "Rate: " << rate << "Hz   -  ready to begin test" <<endl;
			break;
		}
	}

	// Loop over all thread settings in set
	cout << "Testing " << mThreadSet.size() << " Nthread settings with " << mNsamples << " samples each" << endl;
	map< uint32_t, vector<float> > samples;
	map< uint32_t, std::pair<float,float> > rates; // key=nthreads  val.first=rate in Hz, val.second=rms of rate in Hz
	for( auto nthreads : mThreadSet ){
		cout << "Setting NTHREADS = " << nthreads << " ..." <<endl;
		tm->SetNJThreads( nthreads );

		// Loop for at most 60 seconds waiting for the number of threads to update
		for(int i=0; i<60; i++){
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
			if( tm->GetNJThreads() == nthreads ) break;
		}

		// Acquire mNsamples instantaneous rate measurements. The
		// GetInstantaneousRate method will only update every 0.5
		// seconds so we just wait for 1 second between samples to
		// ensure independent measurements.
		double sum  = 0;
		double sum2 = 0;
		for(uint32_t isample=0; isample<mNsamples; isample++){
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
			auto rate = mApp->GetInstantaneousRate();
			samples[nthreads].push_back(rate);

			sum  += rate;
			sum2 += rate*rate;
			double N = (double)(isample+1);
			double avg = sum/N;
			double rms = sqrt( (sum2 + N*avg*avg - 2.0*avg*sum)/N );
			rates[nthreads].first  = avg;  // overwrite with updated value after each sample
			rates[nthreads].second = rms;  // overwrite with updated value after each sample

			cout << "nthreads=" << tm->GetNJThreads() << "  rate=" << rate << "Hz";
			if( N>1 ) cout << "  (avg = " << avg << " +/- " << rms/sqrt(N) << " Hz)";
			cout << endl;
		}
	}

	// Write results to files
	LOG_INFO(mLogger) << "Writing test results to: " << mOutputDirName << LOG_END;
	mkdir(mOutputDirName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::ofstream ofs1(mOutputDirName+"/samples.dat");
	ofs1 << "# nthreads     rate" << endl;
	for( auto p : samples ){
		auto nthreads = p.first;
		for( auto rate: p.second ) ofs1 << std::setw(7) << nthreads << " " << std::setw(12) << std::setprecision(1) << std::fixed << rate << endl;
	}
	ofs1.close();

	std::ofstream ofs2(mOutputDirName+"/rates.dat");
	ofs2 << "# nthreads  avg_rate       rms" << endl;
	for( auto p : rates ){
		auto nthreads = p.first;
		auto avg_rate = p.second.first;
		auto rms      = p.second.second;
		ofs2 << std::setw(7 ) << nthreads << " ";
		ofs2 << std::setw(12) << std::setprecision(1) << std::fixed << avg_rate << " ";
		ofs2 << std::setw(10) << std::setprecision(1) << std::fixed << rms << endl;
	}
	ofs2.close();

	CopyToOutputDir("${JANA_HOME}/src/plugins/JTest/plot_rate_vs_nthreads.py");

	cout << "Testing finished" << endl;
	mApp->Quit();
}

//---------------------------------
// CopyToOutputDir
//
// Copy the specified output file to the currently set mOutputDirName.
// Strings with a format of ${Name} will be substituted with the
// corresponding environment variable.
//---------------------------------
void JTestMain::CopyToOutputDir(std::string filename)
{
	// Substitute environment variables in given filename
	string new_fname = filename;
	while( auto pos_start=new_fname.find("${") != new_fname.npos ){
		auto pos_end = new_fname.find( "}", pos_start+3 );
		if( pos_end != new_fname.npos ){

			string envar_name = new_fname.substr(pos_start+1, pos_end-pos_start-1);
			LOG_DEBUG(mLogger) << "Looking for env var '" << envar_name 
				           << "'" << LOG_END;

			auto envar = getenv( envar_name.c_str() );
			if( envar ) {
				new_fname.replace( pos_start-1, pos_end+2-pos_start, envar);
			}else{
				LOG_ERROR(mLogger) << "Environment variable '" 
					           << envar_name 
						   << "' not set. Cannot copy " 
						   << filename << LOG_END;
				return;
			}
		}else{
			LOG_ERROR(mLogger) << "Error in string format: " 
				           << filename << LOG_END;
		}
	}

	// Extract filename without path
	string base_fname = new_fname;
	if( auto pos = base_fname.rfind("/") ) base_fname.erase(0, pos);

	// Copy file
	LOG_INFO(mLogger) << "Copying " << new_fname << " -> " << mOutputDirName << LOG_END;
	std::ifstream src(new_fname, std::ios::binary);
	std::ofstream dst(mOutputDirName + "/" + base_fname, std::ios::binary);
	dst << src.rdbuf();
}

