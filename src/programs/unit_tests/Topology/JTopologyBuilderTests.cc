
#include "JANA/JApplicationFwd.h"
#include "JANA/Topology/JEventPool.h"
#include "JANA/Topology/JEventTapArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JMultilevelSourceArrow.h>
#include <JANA/JEventProcessor.h>


class DeinterleavedProc : public JEventProcessor {
public:
    DeinterleavedProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent& event) override {
        LOG_INFO(GetLogger()) << "Consuming " << event.GetEventStamp();
    }
};


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


