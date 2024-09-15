
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Engine/JWorker.h>
#include <JANA/Engine/JPerfSummary.h>
#include <JANA/JService.h>

#include <vector>

class JArrowProcessingController : public JService {
public:

    JArrowProcessingController() {
        SetPrefix("jana");
    }
    ~JArrowProcessingController() override;
    void acquire_services(JServiceLocator *) override;

    void initialize();
    void run(size_t nthreads);
    void scale(size_t nthreads);
    void request_pause();
    void wait_until_paused();
    void request_stop();
    void wait_until_stopped();

    bool is_stopped();
    bool is_finished();
    bool is_timed_out();
    bool is_excepted();

    JArrowMetrics::Status execute_arrow(int);

    std::vector<JException> get_exceptions() const;

    std::unique_ptr<const JPerfSummary> measure_performance();

    void print_report();
    void print_final_report();

    // This is so we can test
    inline JScheduler* get_scheduler() { return m_scheduler; }


private:
    std::shared_ptr<JTopologyBuilder> m_topology;

    using jclock_t = std::chrono::steady_clock;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;

    JPerfSummary m_perf_summary;
    JScheduler* m_scheduler = nullptr;

    std::vector<JWorker*> m_workers;
};

