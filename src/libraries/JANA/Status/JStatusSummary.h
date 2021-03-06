
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_J_STATUS_SUMMARY_H
#define JANA2_J_STATUS_SUMMARY_H

#include <JANA/Status/JComponentSummary.h>
#include <JANA/Status/JPerfSummary.h>

#include <map>
#include <JANA/JException.h>

struct JApplicationSummary {
    bool initialized;
    bool quitting;
    bool draining_queues;
    bool skip_join;
    int exit_code;
};

struct JStatusSummary {
    JApplicationSummary application_summary;
    JComponentSummary component_summary;
    JPerfSummary performance_summary;
    std::map<int, std::string> backtraces;
    JException current_exception;
};


#endif //JANA2_J_STATUS_SUMMARY_H
