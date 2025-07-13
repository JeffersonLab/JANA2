
#include "JANA/JApplicationFwd.h"
#include "JANA/Topology/JEventPool.h"
#include "JANA/Topology/JEventTapArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JMultilevelArrow.h>
#include <JANA/Topology/JMultilevelSourceArrow.h>
#include <JANA/Topology/JDeinterleaveArrow.h>
#include <JANA/JEventProcessor.h>

class MySrc : public JMultilevelArrow {
public:
    size_t event_count = 0;

    virtual void Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel& next_input_level, JArrow::FireResult& result) override {

        // Cycle through each of the input levels one after another
        input->SetEventNumber(event_count);
        event_count += 1; // Avoid ever requesting 2 of the same event level in a row
        next_input_level = GetLevels().at(event_count % GetLevels().size()); // Switch to the next event level next time
        outputs.push_back(input);
        LOG_INFO(get_logger()) << "Producing " << toString(input->GetLevel()) << " number " << input->GetEventNumber() << ". Next input level=" << toString(next_input_level);

        if (event_count > 9) {
            result = FireResult::Finished; // TODO: This is SuccessFinished, not FailureFinished
        }
        else {
            result = FireResult::KeepGoing;
        }
    }
};

class InterleavedProc : public JEventProcessor {
public:
    InterleavedProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent& event) override {
        LOG_INFO(GetLogger()) << "Consuming " << event.GetEventStamp();
    }
};

void configure_multilevel_source_topology(JTopologyBuilder& builder) {

    auto run_pool = new JEventPool(builder.m_components, 1, 1, JEventLevel::Run);
    auto controls_pool = new JEventPool(builder.m_components, 1, 1, JEventLevel::SlowControls);
    auto physics_pool = new JEventPool(builder.m_components, 1, 1, JEventLevel::PhysicsEvent);

    builder.pools.push_back(run_pool);
    builder.pools.push_back(controls_pool);
    builder.pools.push_back(physics_pool);

    physics_pool->AttachForwardingPool(controls_pool);
    physics_pool->AttachForwardingPool(run_pool);

    auto* src_arrow = new MySrc;
    src_arrow->set_name("MultilevelArrow");
    src_arrow->ConfigurePorts(JMultilevelArrow::Style::AllToOne, {JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::PhysicsEvent});
    src_arrow->attach(run_pool, src_arrow->GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::In));
    src_arrow->attach(controls_pool, src_arrow->GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::In));
    src_arrow->attach(physics_pool, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::In));

    builder.arrows.push_back(src_arrow);

    JEventTapArrow* tap_arrow = new JEventTapArrow("InterleavedTapArrow");
    for (auto proc : builder.m_components->get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }
    builder.connect(src_arrow, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::Out), tap_arrow, tap_arrow->EVENT_IN);
    builder.queues.at(0)->Scale(3); // TODO: This is a hack

    tap_arrow->attach(physics_pool, tap_arrow->EVENT_OUT);
    builder.arrows.push_back(tap_arrow);
}




TEST_CASE("MultilevelArrowCustomTopology") {

    JApplication app;
    //app.SetParameterValue("jana:loglevel", "debug");
    app.Add(new InterleavedProc);
    auto builder = app.GetService<JTopologyBuilder>();
    builder->set_configure_fn(configure_multilevel_source_topology);
    app.Run();

}

void configure_deinterleave_topology(JTopologyBuilder& builder) {

    // Warning: max_inflight_events:run, 
    auto run_pool = new JEventPool(builder.m_components, 2, 1, JEventLevel::Run);
    auto controls_pool = new JEventPool(builder.m_components, 2, 1, JEventLevel::SlowControls);
    auto physics_pool = new JEventPool(builder.m_components, 4, 1, JEventLevel::PhysicsEvent);

    builder.pools.push_back(run_pool);
    builder.pools.push_back(controls_pool);
    builder.pools.push_back(physics_pool);

    physics_pool->AttachForwardingPool(controls_pool);
    physics_pool->AttachForwardingPool(run_pool);

    auto* src_arrow = new MySrc;
    src_arrow->set_name("MultilevelSource");
    src_arrow->ConfigurePorts(JMultilevelArrow::Style::AllToOne, {JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::PhysicsEvent});
    src_arrow->attach(run_pool, src_arrow->GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::In));
    src_arrow->attach(controls_pool, src_arrow->GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::In));
    src_arrow->attach(physics_pool, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::In));

    // Problem: This design forces us to have a minimum of TWO events in flight because otherwise we can't evict if we have two of the same parent events in a row

    auto* deil_arrow = new JDeinterleaveArrow;
    deil_arrow->set_name("Deinterleave");
    deil_arrow->SetLevels({JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::PhysicsEvent});
    deil_arrow->attach(run_pool, deil_arrow->GetPortIndex(JEventLevel::Run, JMultilevelArrow::Direction::Out));
    deil_arrow->attach(controls_pool, deil_arrow->GetPortIndex(JEventLevel::SlowControls, JMultilevelArrow::Direction::Out));

    JEventTapArrow* tap_arrow = new JEventTapArrow("DeinterleavedTap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.connect(src_arrow, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::Out),
                    deil_arrow, deil_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::In));


    builder.connect(deil_arrow, deil_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelArrow::Direction::Out),
                    tap_arrow, tap_arrow->EVENT_IN);

    builder.queues.at(0)->Scale(8); // Queue capacity = N(Run) + N(SlowControls) + N(PhysicsEvent)
    builder.queues.at(1)->Scale(4); // Queue capacity = N(PhysicsEvent)

    tap_arrow->attach(physics_pool, tap_arrow->EVENT_OUT);

    builder.arrows.push_back(src_arrow);
    builder.arrows.push_back(deil_arrow);
    builder.arrows.push_back(tap_arrow);
}

class DeinterleavedProc : public JEventProcessor {
public:
    DeinterleavedProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent& event) override {
        LOG_INFO(GetLogger()) << "Consuming " << event.GetEventStamp();
    }
};


TEST_CASE("DeinterleaveArrowCustomTopology") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "debug");
    app.Add(new DeinterleavedProc);
    auto builder = app.GetService<JTopologyBuilder>();
    builder->set_configure_fn(configure_deinterleave_topology);
    app.Run();
}

class MyMultiSource : public JEventSource {
public:
    MyMultiSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        SetEventLevels({JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::PhysicsEvent});
    }
    Result Emit(JEvent& event) override {
        auto count = GetEmittedEventCount();
        const auto& levels = GetEventLevels();
        SetNextEventLevel(levels.at((count+1) % levels.size()));
        LOG_INFO(GetLogger()) << "Emitting " << event.GetEventStamp();
        return Result::Success; // Assume that source can peek ahead to request a different level
    }
};

void configure_multisource_topology(JTopologyBuilder& builder) {

    auto run_pool = new JEventPool(builder.m_components, 1, 1, JEventLevel::Run);
    auto controls_pool = new JEventPool(builder.m_components, 2, 1, JEventLevel::SlowControls);
    auto physics_pool = new JEventPool(builder.m_components, 4, 1, JEventLevel::PhysicsEvent);

    builder.pools.push_back(run_pool);
    builder.pools.push_back(controls_pool);
    builder.pools.push_back(physics_pool);

    physics_pool->AttachForwardingPool(controls_pool);
    physics_pool->AttachForwardingPool(run_pool);

    auto* src_arrow = new JMultilevelSourceArrow;
    src_arrow->set_name("MultilevelSource");
    src_arrow->SetEventSource(builder.m_components->get_evt_srces().at(0));
    src_arrow->attach(run_pool, src_arrow->GetPortIndex(JEventLevel::Run, JMultilevelSourceArrow::Direction::In));
    src_arrow->attach(controls_pool, src_arrow->GetPortIndex(JEventLevel::SlowControls, JMultilevelSourceArrow::Direction::In));
    src_arrow->attach(physics_pool, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelSourceArrow::Direction::In));

    src_arrow->attach(run_pool, src_arrow->GetPortIndex(JEventLevel::Run, JMultilevelSourceArrow::Direction::Out));
    src_arrow->attach(controls_pool, src_arrow->GetPortIndex(JEventLevel::SlowControls, JMultilevelSourceArrow::Direction::Out));

    JEventTapArrow* tap_arrow = new JEventTapArrow("DeinterleavedTap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.connect(src_arrow, src_arrow->GetPortIndex(JEventLevel::PhysicsEvent, JMultilevelSourceArrow::Direction::Out),
                    tap_arrow, tap_arrow->EVENT_IN);

    builder.queues.at(0)->Scale(4); // Queue capacity = N(PhysicsEvent)

    tap_arrow->attach(physics_pool, tap_arrow->EVENT_OUT);

    builder.arrows.push_back(src_arrow);
    builder.arrows.push_back(tap_arrow);
}


TEST_CASE("MultilevelSourceCustomTopology") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", 10);
    app.Add(new MyMultiSource);
    app.Add(new DeinterleavedProc);
    auto builder = app.GetService<JTopologyBuilder>();
    builder->set_configure_fn(configure_multisource_topology);
    app.Run();
}


