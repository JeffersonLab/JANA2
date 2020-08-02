
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JPROCESSINGCONTROLLER_H
#define JANA2_JPROCESSINGCONTROLLER_H

#include <vector>
#include <unistd.h>
#include <memory>

#include <JANA/Services/JServiceLocator.h>
#include <JANA/Status/JPerfSummary.h>

class JProcessingController : public JService {
public:

    virtual ~JProcessingController() = default;

    virtual void initialize() = 0;
    virtual void run(size_t nthreads) = 0;
    virtual void scale(size_t nthreads) = 0;
    virtual void request_stop() = 0;
    virtual void wait_until_stopped() = 0;
    virtual bool is_stopped() = 0;
    virtual bool is_finished() = 0;

    virtual std::unique_ptr<const JPerfSummary> measure_performance() = 0;

    virtual void print_report() = 0;
    virtual void print_final_report() = 0;
};

#endif //JANA2_JPROCESSINGCONTROLLER_H

