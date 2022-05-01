
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JDEBUGPROCESSINGCONTROLLER_H
#define JANA2_JDEBUGPROCESSINGCONTROLLER_H

#include <JANA/Services/JProcessingController.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/JLogger.h>

#include <thread>
#include <atomic>

class JDebugProcessingController : public JProcessingController {
public:

    explicit JDebugProcessingController(JComponentManager* jcm);
    ~JDebugProcessingController() override;
    void acquire_services(JServiceLocator*) override;

    void initialize() override;
    void run(size_t nthreads) override;
    void scale(size_t nthreads) override;
    void request_stop() override;
    void wait_until_stopped() override;

    bool is_stopped() override;
    bool is_finished() override;
    bool is_timed_out() override;

    std::unique_ptr<const JPerfSummary> measure_performance() override;

    void print_report() override;
    void print_final_report() override;

private:

    void run_worker();

    bool m_stop_requested = false;
    bool m_stop_achieved = false;
    bool m_finish_achieved = false;

    std::atomic_uint m_total_active_workers {0};
    std::atomic_ullong m_total_events_emitted {0};
    std::atomic_ullong m_total_events_processed {0}; // Grows monotonically with multiple calls to Run, Stop, Scale

    std::vector<std::thread*> m_workers;
    JLogger m_logger = JLogger();
    JPerfMetrics m_perf_metrics;
    JComponentManager* m_component_manager = nullptr;

};

#endif //JANA2_JDEBUGPROCESSINGCONTROLLER_H
