
#pragma once
#include <JANA/JService.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Utils/JApplicationInspector.h>

class JEngine : public JService {
private:

    Service<JComponentManager> m_component_manager {this};
    Service<JTopologyBuilder> m_topology_builder {this};
    Service<JParameterManager> m_params {this};
    Service<JArrowProcessingController> m_processing_controller {this};

    Parameter<int> m_ticker_interval_ms {this, "jana:ticker_interval", 1000,
        "Controls the ticker interval (in ms)"};

    Parameter<bool> m_extended_report {this, "jana:extended_report", false, 
        "Controls whether the ticker shows simple vs detailed performance metrics"};

    bool m_inspecting = false;
    bool m_quitting = false;
    bool m_draining_queues = false;
    bool m_skip_join = false;
    bool m_ticker_on = true;
    bool m_timeout_on = true;
    int  m_desired_nthreads;
    std::atomic_int m_sigint_count = 0;
    std::mutex m_status_mutex;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_last_measurement;
    std::unique_ptr<const JPerfSummary> m_perf_summary;

public:
    void Init() override;
    void initialize();
    void Run(bool wait_until_finished = true);
    void Scale(int nthreads);
    void RequestInspection();
    void RequestPause();
    void WaitUntilPaused();
    void RequestStop();
    void WaitUntilStopped();
    void HandleSigint();


    void SetTicker(bool ticker_on = true);
    bool IsTickerEnabled();
    void SetTimeoutEnabled(bool enabled = true);
    bool IsTimeoutEnabled();
    void PrintStatus();
    void PrintFinalReport();
    uint64_t GetNThreads();
    uint64_t GetNEventsProcessed();
    float GetIntegratedRate();
    float GetInstantaneousRate();
    JPerfSummary GetStatus();

    bool IsQuitting() { return m_quitting; }
    bool IsDrainingQueues() { return m_draining_queues; }


};
