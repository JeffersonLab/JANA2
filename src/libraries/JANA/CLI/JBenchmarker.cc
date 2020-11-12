
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JBenchmarker.h"

#include <JANA/Utils/JCpuInfo.h>

#include <fstream>
#include <cmath>
#include <sys/stat.h>

JBenchmarker::JBenchmarker(JApplication* app) : _app(app) {

    _max_threads = JCpuInfo::GetNumCpus();
    _logger = JLoggingService::logger("JBenchmarker");

    auto params = app->GetJParameterManager();

    params->SetParameter("NEVENTS", 0);
    // Prevent users' choice of events from interfering with everything

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


JBenchmarker::~JBenchmarker() {}


void JBenchmarker::RunUntilFinished() {

    _app->SetTicker(false);
    _app->Run(false);

    // Wait for events to start flowing indicating the source is primed
    for (int i = 0; i < 5; i++) {
        std::cout << "Waiting for event source to start producing ... rate: " << _app->GetInstantaneousRate()
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto rate = _app->GetInstantaneousRate();
        if (rate > 10.0) {
            std::cout << "Rate: " << rate << "Hz   -  ready to begin test" << std::endl;
            break;
        }
    }
    // Loop over all thread settings in set
    std::map<uint32_t, std::vector<double> > samples;
    std::map<uint32_t, std::pair<double, double> > rates; // key=nthreads  val.first=rate in Hz, val.second=rms of rate in Hz
    for (uint32_t nthreads = _min_threads; nthreads <= _max_threads && !_app->IsQuitting(); nthreads += _thread_step) {

        std::cout << "Setting NTHREADS = " << nthreads << " ..." << std::endl;
        _app->Scale(nthreads);

        // Loop for at most 60 seconds waiting for the number of threads to update
        for (int i = 0; i < 60; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (_app->GetNThreads() == nthreads) break;
        }

        // Acquire mNsamples instantaneous rate measurements. The
        // GetInstantaneousRate method will only update every 0.5
        // seconds so we just wait for 1 second between samples to
        // ensure independent measurements.
        double sum = 0;
        double sum2 = 0;
        for (uint32_t isample = 0; isample < _nsamples && !_app->IsQuitting(); isample++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            auto rate = _app->GetInstantaneousRate();
            samples[nthreads].push_back(rate);

            sum += rate;
            sum2 += rate * rate;
            double N = (double) (isample + 1);
            double avg = sum / N;
            double rms = sqrt((sum2 + N * avg * avg - 2.0 * avg * sum) / N);
            rates[nthreads].first = avg;  // overwrite with updated value after each sample
            rates[nthreads].second = rms;  // overwrite with updated value after each sample

            std::cout << "nthreads=" << _app->GetNThreads() << "  rate=" << rate << "Hz";
            std::cout << "  (avg = " << avg << " +/- " << rms / sqrt(N) << " Hz)";
            std::cout << std::endl;
        }
    }

    // Write results to files
    std::cout << "Writing test results to: " << _output_dir << std::endl;
    mkdir(_output_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream ofs1(_output_dir + "/samples.dat");
    ofs1 << "# nthreads     rate" << std::endl;
    for (auto p : samples) {
        auto nthreads = p.first;
        for (auto rate: p.second)
            ofs1 << std::setw(7) << nthreads << " " << std::setw(12) << std::setprecision(1) << std::fixed << rate
                 << std::endl;
    }
    ofs1.close();

    std::ofstream ofs2(_output_dir + "/rates.dat");
    ofs2 << "# nthreads  avg_rate       rms" << std::endl;
    for (auto p : rates) {
        auto nthreads = p.first;
        auto avg_rate = p.second.first;
        auto rms = p.second.second;
        ofs2 << std::setw(7) << nthreads << " ";
        ofs2 << std::setw(12) << std::setprecision(1) << std::fixed << avg_rate << " ";
        ofs2 << std::setw(10) << std::setprecision(1) << std::fixed << rms << std::endl;
    }
    ofs2.close();

    copy_to_output_dir("${JANA_HOME}/bin/jana-plot-scaletest.py");

    std::cout << "Testing finished" << std::endl;
    _app->Quit();
}


void JBenchmarker::copy_to_output_dir(std::string filename) {

    // Substitute environment variables in given filename
    std::string new_fname = filename;
    while (auto pos_start = new_fname.find("${") != new_fname.npos) {
        auto pos_end = new_fname.find("}", pos_start + 3);
        if (pos_end != new_fname.npos) {

            std::string envar_name = new_fname.substr(pos_start + 1, pos_end - pos_start - 1);
            LOG_DEBUG(_logger) << "Looking for env var '" << envar_name
                               << "'" << LOG_END;

            auto envar = getenv(envar_name.c_str());
            if (envar) {
                new_fname.replace(pos_start - 1, pos_end + 2 - pos_start, envar);
            } else {
                LOG_ERROR(_logger) << "Environment variable '"
                                   << envar_name
                                   << "' not set. Cannot copy "
                                   << filename << LOG_END;
                return;
            }
        } else {
            LOG_ERROR(_logger) << "Error in string format: "
                               << filename << LOG_END;
        }
    }

    // Extract filename without path
    std::string base_fname = new_fname;
    if (auto pos = base_fname.rfind("/")) base_fname.erase(0, pos);
    auto out_name = _output_dir + "/" + base_fname;

    // Copy file
    LOG_INFO(_logger) << "Copying " << new_fname << " -> " << _output_dir << LOG_END;
    std::ifstream src(new_fname, std::ios::binary);
    std::ofstream dst(out_name, std::ios::binary);
    dst << src.rdbuf();

    // Change permissions to match source
    struct stat st;
    stat(new_fname.c_str(), &st);
    chmod(out_name.c_str(), st.st_mode);
}













