
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JBENCHMARKER_H
#define JANA2_JBENCHMARKER_H

#include <JANA/JApplication.h>

class JBenchmarker {

    JApplication* _app;
    JLogger _logger = JLoggingService::logger("JBenchmarker");

    size_t _min_threads = 1;
    size_t _max_threads = 0;
    unsigned _thread_step = 1;
    unsigned _nsamples = 15;
    std::string _output_dir = "JANA_Test_Results";

public:
    explicit JBenchmarker(JApplication* app);
    ~JBenchmarker();
    void RunUntilFinished();

private:
    void copy_to_output_dir(std::string filename);
};


#endif //JANA2_JBENCHMARKER_H
