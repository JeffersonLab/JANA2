
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JARROWPROCESSINGCONTROLLER_H
#define JANA2_JARROWPROCESSINGCONTROLLER_H

#include <JANA/Services/JProcessingController.h>

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JWorker.h>
#include <JANA/Engine/JArrowTopology.h>
#include <JANA/Engine/JArrowPerfSummary.h>

#include <vector>

class JArrowProcessingController : public JProcessingController {
public:

    explicit JArrowProcessingController(JArrowTopology* topology) : _topology(topology) {};
    ~JArrowProcessingController() override;
    void acquire_services(JServiceLocator *) override;

    void initialize() override;
    void run(size_t nthreads) override;
    void scale(size_t nthreads) override;
    void request_stop() override;
    void wait_until_stopped() override;

    bool is_stopped() override;
    bool is_finished() override;

    std::unique_ptr<const JPerfSummary> measure_performance() override;
    std::unique_ptr<const JArrowPerfSummary> measure_internal_performance();

    void print_report() override;
    void print_final_report() override;


private:

    using jclock_t = std::chrono::steady_clock;

    JArrowPerfSummary _perf_summary;
    JArrowTopology* _topology;       // Owned by JArrowProcessingController
    JScheduler* _scheduler = nullptr;

    std::vector<JWorker*> _workers;
    JLogger _logger;
    JLogger _worker_logger;
    JLogger _scheduler_logger;

};

#endif //JANA2_JARROWPROCESSINGCONTROLLER_H
