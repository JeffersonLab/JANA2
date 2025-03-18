
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JBenchmarker.h"

#include <JANA/Utils/JCpuInfo.h>

#include <fstream>
#include <cmath>
#include <iomanip>
#include <ios>
#include <sys/stat.h>
#include <iostream>
#include <vector>

JBenchmarker::JBenchmarker(JApplication* app) : m_app(app) {

    m_max_threads = JCpuInfo::GetNumCpus();

    auto params = app->GetJParameterManager();

    m_logger = params->GetLogger("benchmark");

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
            "benchmark:use_log_scale",
            m_use_log_scale,
            "Use log scale (instead of linear)");

    if (m_use_log_scale) {
        // A thread step of 1 won't work for log scale, so in this case we default to 2
        m_thread_step = 2;
    }

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

    LOG_INFO(m_logger) << "Running benchmarker with the following settings:" << std::endl
                       << "    benchmark:minthreads = " << m_min_threads << std::endl
                       << "    benchmark:maxthreads = " << m_max_threads << std::endl
                       << "    benchmark:threadstep = " << m_thread_step << std::endl
                       << "    benchmark:use_log_scale = " << m_use_log_scale << std::endl
                       << "    benchmark:nsamples = " << m_nsamples << std::endl
                       << "    benchmark:resultsdir = " << m_output_dir << std::endl;


    mkdir(m_output_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream samples_file(m_output_dir + "/samples.dat");
    samples_file << "# nthreads     rate" << std::endl;

    std::ofstream rates_file(m_output_dir + "/rates.dat");
    rates_file << "# nthreads  avg_rate       rms" << std::endl;


    std::vector<size_t> nthreads_space;
    if (m_use_log_scale) {
        for (size_t i=m_min_threads; i<m_max_threads; i *= m_thread_step) {
            nthreads_space.push_back(i);
        }
    }
    else {
        // Use linear scale
        for (size_t i=m_min_threads; i<m_max_threads; i += m_thread_step) {
            nthreads_space.push_back(i);
        }
    }
    if (nthreads_space.back() != m_max_threads) {
        nthreads_space.push_back(m_max_threads);
    }

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

    for (size_t nthreads: nthreads_space) {
        if (m_app->IsQuitting()) {
            break;
        }

        m_app->Scale(nthreads);

        // Loop for at most 60 seconds waiting for the number of threads to update
        for (int i = 0; i < 60; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (m_app->GetNThreads() == nthreads) break;
        }

        // Accumulate avg and rms rates for all samples for each nthreads
        double avg = 0;
        double rms = 0;
        double sum = 0;
        double sum2 = 0;

        for (uint32_t isample = 0; isample < m_nsamples && !m_app->IsQuitting(); isample++) {
            // Acquire mNsamples instantaneous rate measurements. The
            // GetInstantaneousRate method will only update every 0.5
            // seconds so we just wait for 1 second between samples to
            // ensure independent measurements.
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            auto rate = m_app->GetInstantaneousRate();

            sum += rate;
            sum2 += rate * rate;
            double N = (double) (isample + 1);
            avg = sum / N; // Overwrite with updated value after each sample
            rms = sqrt((sum2 + N * avg * avg - 2.0 * avg * sum) / N);

            LOG_INFO(m_logger)
                << std::setprecision(2) << std::fixed
                << "nthreads=" << m_app->GetNThreads()
                << "  rate=" << rate << "Hz"
                << "  (avg = " << avg << " +/- " << rms / sqrt(N) << " Hz)" << LOG_END;

            // Write line in sample file
            samples_file << std::setw(7) << nthreads << " "
                         << std::setw(12) << std::setprecision(2) << std::fixed << rate << std::endl;
            samples_file.flush();
        }

        // Write line in rates file
        rates_file << std::setw(7) << nthreads << " "
                   << std::setw(12) << std::setprecision(2) << std::fixed << avg << " "
                   << std::setw(10) << std::setprecision(2) << std::fixed << rms << std::endl;
        rates_file.flush();
    }

    // Close files
    // Hopefully, because we called flush(), the files will be partially filled even if we are SIGKILLed
    // before we reach this point.

    samples_file.close();
    rates_file.close();

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













