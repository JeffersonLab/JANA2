
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JApplication.h>

class JBenchmarker {

    JApplication* m_app;
    JLogger m_logger;

    size_t m_min_threads = 1;
    size_t m_max_threads = 0;
    unsigned m_thread_step = 1;
    unsigned m_nsamples = 15;
    std::string m_output_dir = "JANA_Test_Results";
    bool m_copy_script = true;
    bool m_use_log_scale = false;

public:
    explicit JBenchmarker(JApplication* app);
    ~JBenchmarker();
    void RunUntilFinished();

private:
    void copy_to_output_dir(std::string filename);
};


