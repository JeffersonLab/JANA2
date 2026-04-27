
#include "JANA/JApplicationFwd.h"
#include "JANA/Services/JComponentManager.h"
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

void configure_multisource_topology(JTopologyBuilder& builder, JComponentManager& components) {

    auto* src_arrow = new JMultilevelSourceArrow;
    src_arrow->SetName("src");
    src_arrow->SetEventSource(components.get_evt_srces().at(0));

    JEventTapArrow* tap_arrow = new JEventTapArrow("tap");
    for (auto proc : components.get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.AddArrow(src_arrow);
    builder.AddArrow(tap_arrow);

    builder.ConnectPool("src", "RunIn", JEventLevel::Run);
    builder.ConnectPool("src", "SlowControlsIn", JEventLevel::SlowControls);
    builder.ConnectPool("src", "PhysicsEventIn", JEventLevel::PhysicsEvent);
    builder.ConnectPool("src", "RunOut", JEventLevel::Run);
    builder.ConnectPool("src", "SlowControlsOut", JEventLevel::SlowControls);
    builder.ConnectQueue("src", "PhysicsEventOut", "tap", "in");
    builder.ConnectPool("tap", "out", JEventLevel::PhysicsEvent);
    builder.ConnectPool(JEventLevel::PhysicsEvent, JEventLevel::Run);
    builder.ConnectPool(JEventLevel::PhysicsEvent, JEventLevel::SlowControls);
}


TEST_CASE("MultilevelSourceCustomTopology") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", 10);
    app.Add(new MyMultiSource);
    app.Add(new DeinterleavedProc);
    auto builder = app.GetService<JTopologyBuilder>();
    builder->SetConfigureFn(configure_multisource_topology);
    app.Run();
}


