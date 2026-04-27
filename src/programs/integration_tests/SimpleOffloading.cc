
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <JANA/JFactory.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventMapArrow.h>
#include <JANA/Topology/JEventTapArrow.h>

// This integration test covers the end-to-end testing of a GPU override
// We set this up so that we have the following factory chain:
// A (cpu) -> B (gpu) -> C (cpu)

struct A {int a; };
struct B {int b; };
struct C {int c; };

struct AFac : public JFactory {
  Output<A> a_out {this};
  void Process(const JEvent& event) override {
    LOG_INFO(GetLogger()) << "Running AFac (hopefully on CPU)" << LOG_END;
    A* a = new A;
    a->a = event.GetEventNumber() + 1;
    a_out().push_back(a);
  }
};

struct BFac : public JFactory {
  Input<A> a_in {this};
  Output<B> b_out {this};
  void Process(const JEvent&) override {
    LOG_INFO(GetLogger()) << "Running BFac (hopefully on GPU)" << LOG_END;
    auto* a = a_in->at(0);
    B* b = new B;
    b->b = a->a * 2;
    b_out().push_back(b);
  }
};

struct CFac : public JFactory {
  Input<B> b_in {this};
  Output<C> c_out {this};
  void Process(const JEvent&) override {
    LOG_INFO(GetLogger()) << "Running CFac (hopefully on CPU)" << LOG_END;
    auto* b = b_in->at(0);
    C* c = new C;
    c->c = b->b + 4;
    c_out().push_back(c);
  }
};

struct Proc : public JEventProcessor {
  Input<C> c_in {this};
  Proc() {
    SetCallbackStyle(JFactory::CallbackStyle::ExpertMode);
  }
  void ProcessSequential(const JEvent& event) override {
    LOG_INFO(GetLogger()) << "Retrieving C (hopefully on CPU)" << LOG_END;
    auto* c = c_in->at(0);
    auto evtnr = event.GetEventNumber();
    int expected = ((evtnr + 1) * 2) + 4;
    LOG_INFO(GetLogger()) << "Evt nr " << evtnr << ": " << "Expected " << expected << ", found " << c->c << std::endl;
    REQUIRE(expected == c->c);
  }
};


struct TriggerFactoryInputsArrow : public JArrow {
    std::string unique_name;

    TriggerFactoryInputsArrow() {
        SetName("trigger");
        SetIsParallel(true);
        AddPort("in");
        AddPort("out");
    }

    void Fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override {
        auto* fac = event->GetFactorySet()->GetDatabundle(unique_name)->GetFactory();
        for (auto* input : fac->GetInputs()) {
            input->TriggerFactoryCreate(*event);
        }
        for (auto* input : fac->GetVariadicInputs()) {
            input->TriggerFactoryCreate(*event);
        }
        LOG_DEBUG(m_logger) << "Executed arrow " << GetName() << " for event# " << event->GetEventNumber() << LOG_END;
        outputs[0] = {event, 1};
        output_count = 1;
        status = JArrow::FireResult::KeepGoing;
    }
};

struct OffloadArrow : public JArrow {
    std::string unique_name;
    OffloadArrow() {
        SetName("offload");
        SetIsParallel(false);
        AddPort("in");
        AddPort("out");
    }

    ~OffloadArrow() override {}

    void Fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override {

        event->GetFactorySet()->GetDatabundle(unique_name)->GetFactory()->Create(*event);

        LOG_DEBUG(m_logger) << "Executed arrow " << GetName() << " for event# " << event->GetEventNumber() << LOG_END;
        outputs[0] = {event, 1};
        output_count = 1;
        status = JArrow::FireResult::KeepGoing;
    }
};


void configure_topology(JTopologyBuilder& builder, JComponentManager& components) {

    auto* src_arrow = new JEventSourceArrow("src", components.get_evt_srces());

    TriggerFactoryInputsArrow* trigger_inputs_arrow = new TriggerFactoryInputsArrow;
    trigger_inputs_arrow->unique_name = "B";

    OffloadArrow* offload_arrow = new OffloadArrow;
    offload_arrow->unique_name = "B";

    JEventMapArrow* map_arrow = new JEventMapArrow("map");
    for (auto proc : components.get_evt_procs()) {
        map_arrow->add_processor(proc);
    }

    JEventTapArrow* tap_arrow = new JEventTapArrow("tap");
    for (auto proc : components.get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.AddArrow(src_arrow);
    builder.AddArrow(trigger_inputs_arrow);
    builder.AddArrow(offload_arrow);
    builder.AddArrow(map_arrow);
    builder.AddArrow(tap_arrow);

    builder.ConnectPool("src", "in", JEventLevel::PhysicsEvent);
    builder.ConnectPool("tap", "out", JEventLevel::PhysicsEvent);

    builder.ConnectQueue("src", "out", "trigger", "in");
    builder.ConnectQueue("trigger", "out", "offload", "in");
    builder.ConnectQueue("offload", "out", "map", "in");
    builder.ConnectQueue("map", "out", "tap", "in");
}


TEST_CASE("SimpleOffloading") {
  JApplication app;
  app.Add(new JFactoryGeneratorT<AFac>());
  app.Add(new JFactoryGeneratorT<BFac>());
  app.Add(new JFactoryGeneratorT<CFac>());
  app.Add(new JEventSource);
  app.Add(new Proc);
  app.SetParameterValue("jana:nevents", 3);
  app.SetParameterValue("nthreads", 2);
  app.SetParameterValue("jana:log:show_threadstamp", 1);
  app.SetParameterValue("jana:loglevel", "DEBUG");

  auto builder = app.GetService<JTopologyBuilder>();
  builder->SetConfigureFn(configure_topology);
  app.Run();
}



