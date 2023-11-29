
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JObject.h>
#include <JANA/Engine/JSubeventMailbox.h>
#include <JANA/Engine/JSubeventArrow.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include "JANA/Engine/JTopologyBuilder.h"


struct MyInput : public JObject {
    int x;
    float y;
    int evt = 0;
    int sub = 0;
    MyInput(int x, float y, int evt, int sub) : x(x), y(y), evt(evt), sub(sub) {}
};

struct MyOutput : public JObject {
    float z;
    int evt = 0;
    int sub = 0;
    explicit MyOutput(float z, int evt, int sub) : z(z), evt(evt), sub(sub) {}
};

struct MyProcessor : public JSubeventProcessor<MyInput, MyOutput> {
    MyProcessor() {
        inputTag = "";
        outputTag = "subeventted";
    }
    MyOutput* ProcessSubevent(MyInput* input) override {
        LOG << "Processing subevent " << input->evt << ":" << input->sub << LOG_END;
        return new MyOutput(input->y + (float) input->x, input->evt, input->sub);
    }
};



struct SimpleSource : public JEventSource {
    SimpleSource(std::string name) : JEventSource(name) {};
    void GetEvent(std::shared_ptr<JEvent> event) override {
        auto evt = event->GetEventNumber();
        std::vector<MyInput*> inputs;
        inputs.push_back(new MyInput(22,3.6,evt,0));
        inputs.push_back(new MyInput(23,3.5,evt,1));
        inputs.push_back(new MyInput(24,3.4,evt,2));
        inputs.push_back(new MyInput(25,3.3,evt,3));
        inputs.push_back(new MyInput(26,3.2,evt,4));
        event->Insert(inputs);
        LOG << "Emitting event " << event->GetEventNumber() << LOG_END;
        // throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
    }
};

struct SimpleProcessor : public JEventProcessor {
    std::mutex m_mutex;
    void Process(const std::shared_ptr<const JEvent>& event) {

        std::lock_guard<std::mutex> guard(m_mutex);

        auto outputs = event->Get<MyOutput>();
        // assert(outputs.size() == 4);
        // assert(outputs[0]->z == 25.6f);
        // assert(outputs[1]->z == 26.5f);
        // assert(outputs[2]->z == 27.4f);
        // assert(outputs[3]->z == 28.3f);
        LOG << " Contents of event " << event->GetEventNumber() << LOG_END;
        for (auto output : outputs) {
            LOG << " " << output->evt << ":" << output->sub << " " << output->z << LOG_END;
        }
        LOG << " DONE with contents of event " << event->GetEventNumber() << LOG_END;
    }
};



int main() {

    MyProcessor processor;
    JMailbox<std::shared_ptr<JEvent>*> events_in;
    JMailbox<std::shared_ptr<JEvent>*> events_out;
    JMailbox<SubeventWrapper<MyInput>> subevents_in;
    JMailbox<SubeventWrapper<MyOutput>> subevents_out;

    auto split_arrow = new JSplitArrow<MyInput, MyOutput>("split", &processor, &events_in, &subevents_in);
    auto subprocess_arrow = new JSubeventArrow<MyInput, MyOutput>("subprocess", &processor, &subevents_in, &subevents_out);
    auto merge_arrow = new JMergeArrow<MyInput, MyOutput>("merge", &processor, &subevents_out, &events_out);

    auto parms = new JParameterManager;
    // Some params need to be present BEFORE JApplication is constructed, e.g. log levels are lost
    // parms->SetParameter("log:debug", "JWorker,JScheduler,JArrowProcessingController,JEventProcessorArrow");
    JApplication app(parms);
    app.SetTimeoutEnabled(false);
    app.SetTicker(false);

    auto source = new SimpleSource("simpleSource");
    source->SetNEvents(10);  // limit ourselves to 10 events. Note that the 'jana:nevents' param won't work
                             // here because we aren't using JComponentManager to manage the EventSource

    auto topology = app.GetService<JTopologyBuilder>()->create_empty();
    auto source_arrow = new JEventSourceArrow("simpleSource",
                                              {source},
                                              &events_in,
                                              topology->event_pool);
    auto proc_arrow = new JEventProcessorArrow("simpleProcessor", &events_out, nullptr, topology->event_pool);
    proc_arrow->add_processor(new SimpleProcessor);

    topology->arrows.push_back(source_arrow);
    topology->sources.push_back(source_arrow);
    topology->arrows.push_back(split_arrow);
    topology->arrows.push_back(subprocess_arrow);
    topology->arrows.push_back(merge_arrow);
    topology->arrows.push_back(proc_arrow);
    topology->sinks.push_back(proc_arrow);

    source_arrow->attach(split_arrow);
    split_arrow->attach(subprocess_arrow);
    subprocess_arrow->attach(merge_arrow);
    merge_arrow->attach(proc_arrow);

    app.Run(true);

}

