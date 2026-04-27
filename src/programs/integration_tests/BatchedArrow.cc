#include <catch.hpp>

#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventTapArrow.h>
#include <deque>

class BatchedArrow : public JArrow {

    size_t m_batch_size = 5;
    std::deque<JEvent*> m_batched_events;

public:
    BatchedArrow() {
        SetName("BatchedArrow");
        SetIsParallel(false);
        AddPort("in");
        AddPort("out");
    }

    void SetBatchSize(int batch_size) { m_batch_size = batch_size; }

    virtual void Batch(const JEvent& evt) {
        GetLogger() << "Batching event " << evt.GetEventNumber();
    }

    virtual void Process() {
        GetLogger() << "Processing batch containing:";
        for (auto* evt : m_batched_events) {
            GetLogger() << "    " << evt->GetEventNumber();
        }
    }

    virtual void Unbatch(const JEvent& evt) {
        GetLogger() << "Unbatching event " << evt.GetEventNumber();
    }

    void Fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override {

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
            GetLogger() << "NOT releasing batch yet";
            m_next_input_port = 0;
            output_count = 0;
            status = JArrow::FireResult::KeepGoing;
        }
        else {
            for (int i=0; i<2 && !m_batched_events.empty(); ++i) {
                JEvent* event = m_batched_events.front();
                m_batched_events.pop_front();
                GetLogger() << "Releasing event " << event->GetEventNumber();
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


void configure_batched_topology(JTopologyBuilder& builder, JComponentManager& component_manager) {

    auto* src_arrow = new JEventSourceArrow("PhysicsEventSource", component_manager.get_evt_srces());

    BatchedArrow* batched_arrow = new BatchedArrow;

    JEventTapArrow* tap_arrow = new JEventTapArrow("PhysicsEventTap");
    for (auto proc : component_manager.get_evt_procs()) {
        tap_arrow->add_processor(proc);
    }

    builder.AddArrow(src_arrow);
    builder.AddArrow(batched_arrow);
    builder.AddArrow(tap_arrow);
    builder.ConnectPool("PhysicsEventSource", "in", JEventLevel::PhysicsEvent);
    builder.ConnectQueue("PhysicsEventSource", "out", "BatchedArrow", "in");
    builder.ConnectQueue("BatchedArrow", "out", "PhysicsEventTap", "in");
    builder.ConnectPool("PhysicsEventTap", "out", JEventLevel::PhysicsEvent);
}



TEST_CASE("BatchedArrow") {
  JApplication app;
  app.Add(new JEventSource);
  app.Add(new BatchedProc);
  app.SetParameterValue("jana:nevents", 49);
  app.SetParameterValue("nthreads", 1);
  app.SetParameterValue("jana:max_inflight_events", 20);
  app.SetParameterValue("jana:log:show_threadstamp", 1);
  app.SetParameterValue("jana:loglevel", "TRACE");

  app.GetService<JTopologyBuilder>()->SetConfigureFn(configure_batched_topology);
  app.Run();
}



