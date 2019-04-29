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
// Author: Nathan Brei
//

#include "JBenchmarker.h"

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

    _watcher_thread = new std::thread(&JBenchmarker::run_thread, this);
}


JBenchmarker::~JBenchmarker() {
    if (_watcher_thread != nullptr) {
        _watcher_thread->join();
        delete _watcher_thread;
    }
}


void JBenchmarker::run_thread() {

    _app->SetTicker(false);

    // Wait for events to start flowing indicating the source is primed
    for (int i = 0; i < 60; i++) {
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
    std::map<uint32_t, std::vector<float> > samples;
    std::map<uint32_t, std::pair<float, float> > rates; // key=nthreads  val.first=rate in Hz, val.second=rms of rate in Hz
    for (uint32_t nthreads = _min_threads; nthreads <= _max_threads; nthreads += _thread_step) {

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
        for (uint32_t isample = 0; isample < _nsamples; isample++) {
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
    LOG_INFO(_logger) << "Writing test results to: " << _output_dir << LOG_END;
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

    copy_to_output_dir("${JANA_HOME}/src/plugins/JTest/plot_rate_vs_nthreads.py");

    std::cout << "Testing finished" << std::endl;
    _app->Quit();
}


void JBenchmarker::copy_to_output_dir(std::string filename) {

    // Substitute environment variables in given filename
    string new_fname = filename;
    while (auto pos_start = new_fname.find("${") != new_fname.npos) {
        auto pos_end = new_fname.find("}", pos_start + 3);
        if (pos_end != new_fname.npos) {

            string envar_name = new_fname.substr(pos_start + 1, pos_end - pos_start - 1);
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
    string base_fname = new_fname;
    if (auto pos = base_fname.rfind("/")) base_fname.erase(0, pos);

    // Copy file
    LOG_INFO(_logger) << "Copying " << new_fname << " -> " << _output_dir << LOG_END;
    std::ifstream src(new_fname, std::ios::binary);
    std::ofstream dst(_output_dir + "/" + base_fname, std::ios::binary);
    dst << src.rdbuf();
}














