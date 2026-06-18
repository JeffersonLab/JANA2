
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JService.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Utils/JBacktrace.h>

#include <chrono>
#include <ctime>

class JExecutionEngine : public JService {

public:
    using clock_t = std::chrono::steady_clock;

    enum class RunStatus { Paused, Running, Pausing, Draining, Failed, Finished };

    struct Perf {
        RunStatus runstatus;
        size_t thread_count;
        size_t event_count;
        size_t uptime_ms;
        double throughput_hz;
        JEventLevel event_level;
    };

public:
    JExecutionEngine() = default;
    virtual ~JExecutionEngine() = default;

    virtual void RunTopology() = 0;
    virtual void PauseTopology() = 0;
    virtual void DrainTopology() = 0;
    virtual void FinishTopology() = 0;
    virtual void ScaleWorkers() = 0;
    virtual void ScaleWorkers(size_t nthreads) = 0;
    virtual void RunSupervisor() = 0;

    virtual JArrow::FireResult Fire(size_t arrow_id, size_t location_id=0) = 0;

    virtual Perf GetPerf() = 0;
    virtual RunStatus GetRunStatus() = 0;
    virtual void SetTickerEnabled(bool ticker_on) = 0;
    virtual bool IsTickerEnabled() const = 0;
    virtual void SetTimeoutEnabled(bool timeout_on) = 0;
    virtual bool IsTimeoutEnabled() const = 0;
    virtual void RequestInspector() = 0;

    virtual void HandleSIGINT() = 0;
    virtual void HandleSIGUSR1() = 0;
    virtual void HandleSIGUSR2() = 0;
    virtual void HandleSIGTSTP() = 0;
};

std::string ToString(JExecutionEngine::RunStatus status);


