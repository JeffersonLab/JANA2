
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JBenchmarker.h"

#include <JANA/Utils/JCpuInfo.h>

#include <fstream>
#include <cmath>
#include <sys/stat.h>
#include <iostream>

JBenchmarker::JBenchmarker(JApplication* app) : m_app(app) {

    m_max_threads = JCpuInfo::GetNumCpus();

    auto params = app->GetJParameterManager();

    m_logger = params->GetLogger("JBenchmarker");

    params->SetParameter("jana:nevents", 0);
    // Prevent users' choice of nevents from interfering with everything

    params->SetDefaultParameter(
            "benchmark:nsamples",
            m_nsamples,
            "Number of samples for each benchmark test");

    params->SetDefaultParameter(
            "benchmark:minthreads",
            m_min_threads,
            "Minimum number of threads for benchmark test");

    params->SetDefaultParameter(
            "benchmark:maxthreads",
            m_max_threads,
            "Maximum number of threads for benchmark test");

    params->SetDefaultParameter(
            "benchmark:threadstep",
            m_thread_step,
            "Delta number of threads between each benchmark test");

    params->SetDefaultParameter(
            "benchmark:resultsdir",
            m_output_dir,
            "Output directory name for benchmark test results");

    params->SetDefaultParameter(
            "benchmark:copyscript",
            m_copy_script,
            "Copy plotting script to results dir");

    params->SetParameter("nthreads", m_max_threads);
    // Otherwise JApplication::Scale() doesn't scale up. This is an interesting bug. TODO: Remove me when fixed.
}


JBenchmarker::~JBenchmarker() {}


void JBenchmarker::RunUntilFinished() {

    LOG_INFO(m_logger) << "Running benchmarker with the following settings:\n"
                       << "    benchmark:minthreads = " << m_min_threads << "\n"
                       << "    benchmark:maxthreads = " << m_max_threads << "\n"
                       << "    benchmark:threadstep = " << m_thread_step << "\n"
                       << "    benchmark:nsamples = " << m_nsamples << "\n"
                       << "    benchmark:resultsdir = " << m_output_dir << LOG_END;

    m_app->SetTicker(false);
    m_app->Run(false);

    // Wait for events to start flowing indicating the source is primed
    for (int i = 0; i < 5; i++) {
        LOG_INFO(m_logger) << "Waiting for event source to start producing ... rate: " << m_app->GetInstantaneousRate() << LOG_END;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto rate = m_app->GetInstantaneousRate();
        if (rate > 10.0) {
            LOG_INFO(m_logger) << "Rate: " << rate << "Hz   -  ready to begin test" << LOG_END;
            break;
        }
    }
    // Loop over all thread settings in set
    std::map<uint32_t, std::vector<double> > samples;
    std::map<uint32_t, std::pair<double, double> > rates; // key=nthreads  val.first=rate in Hz, val.second=rms of rate in Hz
    for (uint32_t nthreads = m_min_threads; nthreads <= m_max_threads && !m_app->IsQuitting(); nthreads += m_thread_step) {

        LOG_INFO(m_logger) << "Setting nthreads = " << nthreads << " ..." << LOG_END;
        m_app->Scale(nthreads);

        // Loop for at most 60 seconds waiting for the number of threads to update
        for (int i = 0; i < 60; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (m_app->GetNThreads() == nthreads) break;
        }

        // Acquire mNsamples instantaneous rate measurements. The
        // GetInstantaneousRate method will only update every 0.5
        // seconds so we just wait for 1 second between samples to
        // ensure independent measurements.
        double sum = 0;
        double sum2 = 0;
        for (uint32_t isample = 0; isample < m_nsamples && !m_app->IsQuitting(); isample++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            auto rate = m_app->GetInstantaneousRate();
            samples[nthreads].push_back(rate);

            sum += rate;
            sum2 += rate * rate;
            double N = (double) (isample + 1);
            double avg = sum / N;
            double rms = sqrt((sum2 + N * avg * avg - 2.0 * avg * sum) / N);
            rates[nthreads].first = avg;  // overwrite with updated value after each sample
            rates[nthreads].second = rms;  // overwrite with updated value after each sample

            LOG_INFO(m_logger) << "nthreads=" << m_app->GetNThreads() << "  rate=" << rate << "Hz"
                       << "  (avg = " << avg << " +/- " << rms / sqrt(N) << " Hz)" << LOG_END;
        }
    }

    // Write results to files
    LOG_INFO(m_logger) << "Writing test results to: " << m_output_dir << LOG_END;
    mkdir(m_output_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream ofs1(m_output_dir + "/samples.dat");
    ofs1 << "# nthreads     rate" << std::endl;
    for (auto p : samples) {
        auto nthreads = p.first;
        for (auto rate: p.second)
            ofs1 << std::setw(7) << nthreads << " " << std::setw(12) << std::setprecision(1) << std::fixed << rate
                 << std::endl;
    }
    ofs1.close();

    std::ofstream ofs2(m_output_dir + "/rates.dat");
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

    if (m_copy_script) {
        copy_to_output_dir("${JANA_HOME}/bin/jana-plot-scaletest.py");
        LOG_INFO(m_logger)
            << "Testing finished. To view a plot of test results:\n"
            << "    cd " << m_output_dir
            << "\n    ./jana-plot-scaletest.py\n" << LOG_END;
    }
    else {
        LOG_INFO(m_logger) 
            << "Testing finished. To view a plot of test results:\n"
            << "    cd " << m_output_dir << "\n"
            << "    $JANA_HOME/bin/jana-plot-scaletest.py\n" << LOG_END;
    }
    m_app->Stop(true);
}


void JBenchmarker::copy_to_output_dir(std::string filename) {

    // Substitute environment variables in given filename
    std::string new_fname = filename;
    while (auto pos_start = new_fname.find("${") != new_fname.npos) {
        auto pos_end = new_fname.find("}", pos_start + 3);
        if (pos_end != new_fname.npos) {

            std::string envar_name = new_fname.substr(pos_start + 1, pos_end - pos_start - 1);
            LOG_DEBUG(m_logger) << "Looking for env var '" << envar_name
                                << "'" << LOG_END;

            auto envar = getenv(envar_name.c_str());
            if (envar) {
                new_fname.replace(pos_start - 1, pos_end + 2 - pos_start, envar);
            } else {
                LOG_ERROR(m_logger) << "Environment variable '"
                                    << envar_name
                                    << "' not set. Cannot copy "
                                    << filename << LOG_END;
                return;
            }
        } else {
            LOG_ERROR(m_logger) << "Error in string format: "
                                << filename << LOG_END;
        }
    }

    // Extract filename without path
    std::string base_fname = new_fname;
    if (auto pos = base_fname.rfind("/")) base_fname.erase(0, pos);
    auto out_name = m_output_dir + "/" + base_fname;

    // Copy file
    LOG_INFO(m_logger) << "Copying " << new_fname << " -> " << m_output_dir << LOG_END;
    std::ifstream src(new_fname, std::ios::binary);
    std::ofstream dst(out_name, std::ios::binary);
    dst << src.rdbuf();

    // Change permissions to match source
    struct stat st;
    stat(new_fname.c_str(), &st);
    chmod(out_name.c_str(), st.st_mode);
}













