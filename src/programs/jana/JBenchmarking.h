#pragma once

#include "JApplication.h"



class JBenchmarking {
protected:

    JApplication* _app;
    std::shared_ptr<JLogger> _logger;

    int _min_threads = 1;
    int _max_threads;
    int _thread_step;
    int _nsamples = 15;
    bool _quit = false;

    std::string _output_dir;
    std::thread* _watcher_thread;


public:

    JBenchmarking() {
        _output_dir = "JANA_Test_Results";
        _max_threads = JCpuInfo::GetNumCpus();
    }

    ~JBenchmarking() {
        if(_watcher_thread != nullptr){
            _quit = true;
            _watcher_thread->join();
            delete _watcher_thread;
        }
    }

    void Initialize(JApplication* app) {
        _app = app;
        _logger = app->GetJLogger();
        auto params = app->GetJParameterManager();

        params->SetDefaultParameter(
                "BENCHMARK:NSAMPLES",
                _nsamples,
                "Number of samples for each benchmark test");

        params->SetDefaultParameter(
                "BENCHMARK:MINTHREADS",
                _min_threads,
                "Minimum number of threads for benchmark test");

        params->SetDefaultParameter(
                "BENCHMARK:MAXTHREADS",
                _max_threads,
                "Maximum number of threads for benchmark test");

        params->SetDefaultParameter(
                "BENCHMARK:THREADSTEP",
                _thread_step,
                "Delta number of threads between each benchmark test");

        params->SetDefaultParameter(
                "BENCHMARK:RESULTSDIR",
                _output_dir,
                "Output directory name for benchmark test results");
    }

    void Run() {
        _watcher_thread = new std::thread(&JBenchmarking::TestThread, this);


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
	map< uint32_t, vector<float> > samples;
	map< uint32_t, std::pair<float,float> > rates; // key=nthreads  val.first=rate in Hz, val.second=rms of rate in Hz
	for (uint32_t nthreads=mMinThreads; nthreads<=mMaxThreads; nthreads+=mThreadStep){
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

    void CopyToOutputDir(std::string filename) {

    }
};