#include <catch.hpp>

#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventMapArrow.h>
#include <JANA/Topology/JEventTapArrow.h>
#include <deque>

class BatchedArrow : public JArrow {

    size_t m_batch_size = 5;
    std::deque<JEvent*> m_batched_events;

public:
    BatchedArrow() {
        set_name("BatchedArrow");
        set_is_parallel(false);
        create_ports(1, 1);
    }

    void SetBatchSize(int batch_size) { m_batch_size = batch_size; }

    virtual void Batch(const JEvent& evt) {
        get_logger() << "Batching event " << evt.GetEventNumber();
    }

    virtual void Process() {
        get_logger() << "Processing batch containing:";
        for (auto* evt : m_batched_events) {
            get_logger() << "    " << evt->GetEventNumber();
        }
    }

    virtual void Unbatch(const JEvent& evt) {
        get_logger() << "Unbatching event " << evt.GetEventNumber();
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override {

        bool releasing_batch = (event == nullptr);

        if (event != nullptr) {
            // In this case, we are _filling_ the batch, and processing it once it gets filled
            // Populate inputs
            Batch(*event);

            m_batched_events.push_back(event);

            if (m_batched_events.size() == m_batch_size) {
                releasing_batch = true;
                Process();
                for (JEvent* event : m_batched_events) {
                    Unbatch(*event);
                    // Publish outputs
                }
            }
        }
        if (!releasing_batch) {
            get_logger() << "NOT releasing batch yet";
            m_next_input_port = 0;
            output_count = 0;
            status = JArrow::FireResult::KeepGoing;
        }
        else {
            for (int i=0; i<2 && !m_batched_events.empty(); ++i) {
                JEvent* event = m_batched_events.front();
                m_batched_events.pop_front();
                get_logger() << "Releasing event " << event->GetEventNumber();
                outputs[i] = {event, 1};
                output_count = i+1;
                status = JArrow::FireResult::KeepGoing;
            }

            if (m_batched_events.empty()) {
                m_next_input_port = 0;
            }
            else {
                m_next_input_port = -1;
            }
        }
    }
};

struct BatchedProc : public JEventProcessor {
  std::vector<int> observed_event_numbers;
  BatchedProc() {
    SetCallbackStyle(JFactory::CallbackStyle::ExpertMode);
  }
  void ProcessSequential(const JEvent& event) override {
    LOG_INFO(GetLogger()) << "JEP found event" << event.GetEventNumber();
        observed_event_numbers.push_back(event.GetEventNumber());
  }
  void Finish() override {
    LOG_INFO(GetLogger()) << "BatchedProc observed event numbers";
    for (int x: observed_event_numbers) {
      LOG_INFO(GetLogger()) << "    " << x;
    }
  }
};


void configure_batched_topology(JTopologyBuilder& builder) {

    auto pool = new JEventPool(builder.m_components, 20, 1, JEventLevel::PhysicsEvent);
    builder.pools.push_back(pool);

    auto* src_arrow = new JEventSourceArrow("PhysicsEventSource", builder.m_components->get_evt_srces());

    BatchedArrow* batched_arrow = new BatchedArrow;

    JEventMapArrow* map_arrow = new JEventMapArrow("PhysicsEventMap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        map_arrow->add_processor(proc);
    }

    JEventTapArrow* tap_arrow = new JEventTapArrow("PhysicsEventTap");
    for (auto proc : builder.m_components->get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    src_arrow->attach(pool, src_arrow->EVENT_IN);
    tap_arrow->attach(pool, tap_arrow->EVENT_OUT);

    builder.connect(src_arrow, src_arrow->EVENT_OUT, batched_arrow, 0);
    builder.connect(batched_arrow, 1, tap_arrow, tap_arrow->EVENT_IN);

    builder.queues.at(0)->Scale(20);
    builder.queues.at(1)->Scale(20);

    builder.arrows.push_back(src_arrow);
    builder.arrows.push_back(batched_arrow);
    builder.arrows.push_back(tap_arrow);
}



TEST_CASE("BatchedArrow") {
  JApplication app;
  app.Add(new JEventSource);
  app.Add(new BatchedProc);
  app.SetParameterValue("jana:nevents", 499);
  app.SetParameterValue("nthreads", 1);
  app.SetParameterValue("jana:log:show_threadstamp", 1);
  app.SetParameterValue("jana:loglevel", "TRACE");

  auto builder = app.GetService<JTopologyBuilder>();
  builder->set_configure_fn(configure_batched_topology);
  app.Run();
}



