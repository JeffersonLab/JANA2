
#include "JANA/JApplicationFwd.h"
#include "JANA/Topology/JEventPool.h"
#include "JANA/Topology/JEventTapArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JMultilevelArrow.h>
#include <JANA/JEventProcessor.h>

class MySrc : public JMultilevelArrow {
public:
    size_t event_count = 0;

    virtual void Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel& next_input_level, JArrow::FireResult& result) override {

        // Cycle through each of the input levels one after another
        input->SetEventNumber(event_count);
        next_input_level = GetLevels().at(event_count % GetLevels().size());
        outputs.push_back(input);
        LOG_INFO(get_logger()) << "Producing " << toString(input->GetLevel()) << " number " << input->GetEventNumber();
        event_count += 1;

        if (event_count > 9) {
            result = FireResult::Finished; // TODO: This is SuccessFinished, not FailureFinished
        }
        else {
            result = FireResult::KeepGoing;
        }
    }
};

class MyProc : public JEventProcessor {
public:
    MyProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent& event) override {
        LOG_INFO(GetLogger()) << "Consuming " << toString(event.GetLevel()) << " number " << event.GetEventNumber();
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
    auto* src_arrow = new JMultilevelArrow;
    src_arrow->ConfigurePorts(JMultilevelArrow::Style::AllToOne, {JEventLevel::Run, JEventLevel::Timeslice});

    JApplication app;
    //app.SetParameterValue("jana:loglevel", "debug");
    app.Add(new MyProc);
    auto builder = app.GetService<JTopologyBuilder>();
    builder->set_configure_fn(configure_multilevel_source_topology);
    app.Run();

}
