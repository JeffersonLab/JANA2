
#include <catch.hpp>
#include <chrono>
#include <thread>
#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/Engine/JExecutionEngine.h>

namespace jana::integration_tests::timeslice_unfolder {

struct Hit {
    int ts_nr;
    int hit_nr;
};

// Emits Timeslice-level events, each containing n_hits_per_timeslice hits.
class MyTimesliceSource : public JEventSource {
private:
    Output<Hit> m_hits_out {this};
    int m_total_timeslices;
    int m_hits_per_timeslice;
    int m_ts_emitted = 0;

public:
    MyTimesliceSource(int total_timeslices, int hits_per_timeslice)
        : m_total_timeslices(total_timeslices), m_hits_per_timeslice(hits_per_timeslice)
    {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        SetLevel(JEventLevel::Timeslice);
    }

    Result Emit(JEvent& event) override {
        if (m_ts_emitted >= m_total_timeslices) {
            return Result::FailureFinished;
        }
        event.SetEventNumber(m_ts_emitted);
        for (int i = 0; i < m_hits_per_timeslice; ++i) {
            m_hits_out().push_back(new Hit{m_ts_emitted, i});
        }
        m_ts_emitted++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return Result::Success;
    }
};

// Splits each Timeslice into n_hits_per_timeslice PhysicsEvents, one hit each.
class MyTimesliceUnfolder : public JEventUnfolder {
private:
    Input<Hit> m_hits_in {this};
    Output<Hit> m_hit_out {this};
    int m_hits_per_timeslice;

public:
    explicit MyTimesliceUnfolder(int hits_per_timeslice)
        : m_hits_per_timeslice(hits_per_timeslice)
    {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }

    Result Unfold(const JEvent& parent, JEvent& child, int child_idx) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        child.SetEventNumber(parent.GetEventNumber() * m_hits_per_timeslice + child_idx);
        const Hit* src = m_hits_in->at(child_idx);
        m_hit_out().push_back(new Hit{src->ts_nr, src->hit_nr});
        if (child_idx == m_hits_per_timeslice - 1) {
            return Result::NextChildNextParent;
        }
        return Result::NextChildKeepParent;
    }
};

// Validates that each PhysicsEvent carries exactly the hit it should.
class MyPhysicsEventProcessor : public JEventProcessor {
private:
    Input<Hit> m_hit_in {this};
    int m_hits_per_timeslice;

public:
    explicit MyPhysicsEventProcessor(int hits_per_timeslice)
        : m_hits_per_timeslice(hits_per_timeslice)
    {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void ProcessSequential(const JEvent& event) override {
        int evt_nr = (int) event.GetEventNumber();
        int ts_nr = (int) event.GetParentNumber(JEventLevel::Timeslice);
        int expected_ts_nr = evt_nr / m_hits_per_timeslice;
        int expected_hit_nr = evt_nr % m_hits_per_timeslice;
        REQUIRE(ts_nr == expected_ts_nr);
        REQUIRE(m_hit_in->size() == 1);
        REQUIRE(m_hit_in->at(0)->ts_nr == expected_ts_nr);
        REQUIRE(m_hit_in->at(0)->hit_nr == expected_hit_nr);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};


TEST_CASE("TimesliceUnfolder") {
    const int n_timeslices = 5;
    int n_hits_per_timeslice;

    SECTION("FourEventsPerTimeslice") {
        n_hits_per_timeslice = 4;
    }
    SECTION("ThreeEventsPerTimeslice") {
        n_hits_per_timeslice = 3;
    }

    JApplication app;
    app.SetParameterValue("jana:loglevel", "info");
    app.Add(new MyTimesliceSource(n_timeslices, n_hits_per_timeslice));
    app.Add(new MyTimesliceUnfolder(n_hits_per_timeslice));
    app.Add(new MyPhysicsEventProcessor(n_hits_per_timeslice));
    app.Run();

    auto ee = app.GetService<JExecutionEngine>();
    auto perf = ee->GetPerf();

    REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Finished);

    // Both event levels should have been processed with positive throughput
    REQUIRE(perf.throughputs_hz.count(JEventLevel::Timeslice) == 1);
    REQUIRE(perf.throughputs_hz.count(JEventLevel::PhysicsEvent) == 1);
    REQUIRE(perf.throughputs_hz.at(JEventLevel::Timeslice) > 0);
    REQUIRE(perf.throughputs_hz.at(JEventLevel::PhysicsEvent) > 0);

    // Each timeslice unfolds into n_hits_per_timeslice physics events, so the
    // throughput ratio must equal n_hits_per_timeslice (same uptime denominator)
    REQUIRE(perf.throughputs_hz.at(JEventLevel::PhysicsEvent) ==
            Approx(perf.throughputs_hz.at(JEventLevel::Timeslice) * n_hits_per_timeslice));
}

} // namespace jana::integration_tests::timeslice_unfolder
