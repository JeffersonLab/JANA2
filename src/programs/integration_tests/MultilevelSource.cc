
#include <catch.hpp>
#include <chrono>
#include <thread>
#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <JANA/Engine/JExecutionEngine.h>

namespace jana::integration_tests::multilevel_source {

struct Data {
    int val;
};

class InterleavedSource : public JEventSource {
private:
    Output<Data> m_run_data {this};
    Output<Data> m_controls_data {this};
    Output<Data> m_event_data {this};

    int m_run_period;
    int m_sc_period;
    int m_total_events;

    int m_pe_emitted = 0;
    int m_run_emitted = 0;
    int m_sc_emitted = 0;

    JEventLevel NextNeededLevel() {
        if (m_run_emitted * m_run_period == m_pe_emitted) {
            return JEventLevel::Run;
        } else if (m_sc_emitted * m_sc_period == m_pe_emitted) {
            return JEventLevel::SlowControls;
        } else {
            return JEventLevel::PhysicsEvent;
        }
    }

public:
    InterleavedSource(int run_period, int sc_period, int total_events)
        : m_run_period(run_period), m_sc_period(sc_period), m_total_events(total_events)
    {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        SetParentLevels({JEventLevel::Run, JEventLevel::SlowControls});
        SetLevel(JEventLevel::PhysicsEvent);
        m_run_data.SetLevel(JEventLevel::Run);
        m_controls_data.SetLevel(JEventLevel::SlowControls);
        m_event_data.SetLevel(JEventLevel::PhysicsEvent);
    }

    Result Emit(JEvent& event) override {
        // The arrow reads GetNextInputLevel() unconditionally after every Emit call,
        // so SetNextEventLevel must always be called before returning.

        if (m_pe_emitted >= m_total_events) {
            SetNextEventLevel(JEventLevel::PhysicsEvent);
            return Result::FailureFinished;
        }

        JEventLevel needed = NextNeededLevel();
        SetNextEventLevel(needed);

        if (event.GetLevel() != needed) {
            return Result::FailureLevelChange;
        }

        if (needed == JEventLevel::Run) {
            m_run_data().push_back(new Data{m_run_emitted});
            m_run_emitted++;
        } else if (needed == JEventLevel::SlowControls) {
            m_controls_data().push_back(new Data{m_sc_emitted});
            m_sc_emitted++;
        } else {
            m_event_data().push_back(new Data{m_pe_emitted});
            m_pe_emitted++;
        }

        // Update next level after state has changed
        SetNextEventLevel(m_pe_emitted < m_total_events ? NextNeededLevel() : JEventLevel::PhysicsEvent);
        return Result::Success;
    }
};

class MultilevelProcessor : public JEventProcessor {
private:
    Input<Data> m_run_data {this};
    Input<Data> m_controls_data {this};
    Input<Data> m_event_data {this};

    int m_run_period;
    int m_sc_period;

public:
    MultilevelProcessor(int run_period, int sc_period)
        : m_run_period(run_period), m_sc_period(sc_period)
    {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        m_run_data.SetLevel(JEventLevel::Run);
        m_controls_data.SetLevel(JEventLevel::SlowControls);
        m_event_data.SetLevel(JEventLevel::PhysicsEvent);
    }

    void ProcessSequential(const JEvent& event) override {
        REQUIRE(event.GetLevel() == JEventLevel::PhysicsEvent);

        REQUIRE(m_event_data->size() == 1);
        REQUIRE(m_run_data->size() == 1);
        REQUIRE(m_controls_data->size() == 1);

        int pe_nr = m_event_data->at(0)->val;
        REQUIRE(m_run_data->at(0)->val == pe_nr / m_run_period);
        REQUIRE(m_controls_data->at(0)->val == pe_nr / m_sc_period);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

TEST_CASE("InterleavedSource") {
    const int run_period = 100;
    const int total_events = 200;
    int sc_period;

    SECTION("SlowControls_every_10") {
        sc_period = 10;
    }
    SECTION("SlowControls_every_20") {
        sc_period = 20;
    }

    JApplication app;
    app.SetParameterValue("jana:loglevel", "info");
    app.Add(new InterleavedSource(run_period, sc_period, total_events));
    app.Add(new MultilevelProcessor(run_period, sc_period));
    app.Run();

    auto ee = app.GetService<JExecutionEngine>();
    auto perf = ee->GetPerf();

    REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Finished);

    // All three event levels should have been processed with positive throughput
    REQUIRE(perf.throughputs_hz.count(JEventLevel::PhysicsEvent) == 1);
    REQUIRE(perf.throughputs_hz.count(JEventLevel::Run) == 1);
    REQUIRE(perf.throughputs_hz.count(JEventLevel::SlowControls) == 1);
    REQUIRE(perf.throughputs_hz.at(JEventLevel::PhysicsEvent) > 0);
    REQUIRE(perf.throughputs_hz.at(JEventLevel::Run) > 0);
    REQUIRE(perf.throughputs_hz.at(JEventLevel::SlowControls) > 0);

    // throughput ratios must match the configured event-level ratios exactly,
    // because all levels share the same uptime denominator
    REQUIRE(perf.throughputs_hz.at(JEventLevel::PhysicsEvent) ==
            Approx(perf.throughputs_hz.at(JEventLevel::Run) * run_period));
    REQUIRE(perf.throughputs_hz.at(JEventLevel::PhysicsEvent) ==
            Approx(perf.throughputs_hz.at(JEventLevel::SlowControls) * sc_period));
}

} // namespace jana::integration_tests::multilevel_source
