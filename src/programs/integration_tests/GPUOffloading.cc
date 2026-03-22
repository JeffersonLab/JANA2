
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
  void Process(const JEvent& event) override {
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


void configure_topology(JTopologyBuilder& builder) {

    auto pool = new JEventPool(builder.m_components, 1, 1, JEventLevel::PhysicsEvent);
    builder.pools.push_back(pool);

    auto* src_arrow = new JEventSourceArrow("PhysicsEventSource", builder.m_components->get_evt_srces());
    src_arrow->attach(pool, src_arrow->EVENT_IN);

    JEventMapArrow* map_arrow = new JEventMapArrow("PhysicsEventMap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        map_arrow->add_processor(proc);
    }

    builder.connect(src_arrow, src_arrow->EVENT_OUT, map_arrow, map_arrow->EVENT_IN);

    JEventTapArrow* tap_arrow = new JEventTapArrow("PhysicsEventTap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.connect(map_arrow, map_arrow->EVENT_OUT, tap_arrow, tap_arrow->EVENT_IN);
    tap_arrow->attach(pool, tap_arrow->EVENT_OUT);

    builder.queues.at(0)->Scale(4);
    builder.queues.at(1)->Scale(4);

    builder.arrows.push_back(src_arrow);
    builder.arrows.push_back(map_arrow);
    builder.arrows.push_back(tap_arrow);
}



TEST_CASE("GPUOffloading") {
  JApplication app;
  app.Add(new JFactoryGeneratorT<AFac>());
  app.Add(new JFactoryGeneratorT<BFac>());
  app.Add(new JFactoryGeneratorT<CFac>());
  app.Add(new JEventSource);
  app.Add(new Proc);
  app.SetParameterValue("jana:nevents", 3);
  app.SetParameterValue("nthreads", 2);
  //app.SetParameterValue("jana:loglevel", "TRACE");

  auto builder = app.GetService<JTopologyBuilder>();
  builder->set_configure_fn(configure_topology);
  app.Run();
}

