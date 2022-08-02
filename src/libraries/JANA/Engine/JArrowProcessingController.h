
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

    explicit JArrowProcessingController(std::shared_ptr<JArrowTopology> topology) : m_topology(topology) {};
    ~JArrowProcessingController() override;
    void acquire_services(JServiceLocator *) override;

    void initialize() override;
    void run(size_t nthreads) override;
    void scale(size_t nthreads) override;
    void request_pause();
    void wait_until_paused();
    void request_stop() override;
    void wait_until_stopped() override;

    bool is_stopped() override;
    bool is_finished() override;
    bool is_timed_out() override;

    std::unique_ptr<const JPerfSummary> measure_performance() override;
    std::unique_ptr<const JArrowPerfSummary> measure_internal_performance();

    void print_report() override;
    void print_final_report() override;


private:

    using jclock_t = std::chrono::steady_clock;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;

    JArrowPerfSummary m_perf_summary;
    std::shared_ptr<JArrowTopology> m_topology;       // Owned by JArrowProcessingController
    JScheduler* m_scheduler = nullptr;

    std::vector<JWorker*> m_workers;
    JLogger m_logger;
    JLogger m_worker_logger;
    JLogger m_scheduler_logger;

};

#endif //JANA2_JARROWPROCESSINGCONTROLLER_H
