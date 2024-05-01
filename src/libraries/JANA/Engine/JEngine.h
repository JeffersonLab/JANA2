
#pragma once
#include <JANA/JService.h>

class JEngine : public JService {
private:

    Service<JTopologyBuilder> m_topology_builder {this};
    Service<JParameterManager> m_params {this};

    Parameter<int> m_ticker_interval_ms {this, "jana:ticker_interval", 1000,
        "Controls the ticker interval (in ms)"};

    Parameter<bool> m_extended_report {this, "jana:extended_report", false, 
        "Controls whether the ticker shows simple vs detailed performance metrics"};


    bool m_quitting = false;
    bool m_draining_queues = false;
    bool m_skip_join = false;
    bool m_ticker_on = true;
    bool m_timeout_on = true;
    int  m_desired_nthreads;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_last_measurement;
    JArrowPerfSummary m_perf_summary;
    std::unique_ptr<const JPerfSummary> m_perf_summary;
    void update_status();

public:
    void Init() override;
    void Run(bool wait_until_finished = true);
    void Scale(int nthreads);
    void Pause(bool wait_until_paused = false);
    void Stop(bool wait_until_stopped = false);
    void Resume();
    void Quit(bool skip_join = false);



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


}
