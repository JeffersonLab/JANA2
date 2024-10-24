
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JObject.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventMapArrow.h>
#include <JANA/Topology/JSubeventArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>


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
    SimpleSource() : JEventSource() { 
        SetCallbackStyle(CallbackStyle::ExpertMode); 
    };
    Result Emit(JEvent& event) override {
        auto evt = event.GetEventNumber();
        std::vector<MyInput*> inputs;
        inputs.push_back(new MyInput(22,3.6,evt,0));
        inputs.push_back(new MyInput(23,3.5,evt,1));
        inputs.push_back(new MyInput(24,3.4,evt,2));
        inputs.push_back(new MyInput(25,3.3,evt,3));
        inputs.push_back(new MyInput(26,3.2,evt,4));
        event.Insert(inputs);
        LOG << "Emitting event " << event.GetEventNumber() << LOG_END;
        return Result::Success;
    }
};

struct SimpleProcessor : public JEventProcessor {
    SimpleProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode); 
    }
    void Process(const JEvent& event) {

        std::lock_guard<std::mutex> guard(m_mutex);

        auto outputs = event.Get<MyOutput>();
        // assert(outputs.size() == 4);
        // assert(outputs[0]->z == 25.6f);
        // assert(outputs[1]->z == 26.5f);
        // assert(outputs[2]->z == 27.4f);
        // assert(outputs[3]->z == 28.3f);
        LOG << " Contents of event " << event.GetEventNumber() << LOG_END;
        for (auto output : outputs) {
            LOG << " " << output->evt << ":" << output->sub << " " << output->z << LOG_END;
        }
        LOG << " DONE with contents of event " << event.GetEventNumber() << LOG_END;
    }
};



int main() {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "info");
    app.SetTimeoutEnabled(false);
    app.SetTicker(false);

    auto source = new SimpleSource();
    source->SetNEvents(10);  // limit ourselves to 10 events. Note that the 'jana:nevents' param won't work
                             // here because we aren't using JComponentManager to manage the EventSource
    MyProcessor processor;

    auto topology = app.GetService<JTopologyBuilder>();
    topology->set_configure_fn([&](JTopologyBuilder& builder) {

        JMailbox<JEvent*> events_in;
        JMailbox<JEvent*> events_out;
        JMailbox<SubeventWrapper<MyInput>> subevents_in;
        JMailbox<SubeventWrapper<MyOutput>> subevents_out;

        auto split_arrow = new JSplitArrow<MyInput, MyOutput>("split", &processor, &events_in, &subevents_in);
        auto subprocess_arrow = new JSubeventArrow<MyInput, MyOutput>("subprocess", &processor, &subevents_in, &subevents_out);
        auto merge_arrow = new JMergeArrow<MyInput, MyOutput>("merge", &processor, &subevents_out, &events_out);

        auto source_arrow = new JEventSourceArrow("simpleSource", {source});
        source_arrow->set_input(topology->event_pool);
        source_arrow->set_output(&events_in);

        auto proc_arrow = new JEventMapArrow("simpleProcessor");
        proc_arrow->set_input(&events_out);
        proc_arrow->set_output(topology->event_pool);
        proc_arrow->add_processor(new SimpleProcessor);

        builder.arrows.push_back(source_arrow);
        builder.arrows.push_back(split_arrow);
        builder.arrows.push_back(subprocess_arrow);
        builder.arrows.push_back(merge_arrow);
        builder.arrows.push_back(proc_arrow);

        source_arrow->attach(split_arrow);
        split_arrow->attach(subprocess_arrow);
        subprocess_arrow->attach(merge_arrow);
        merge_arrow->attach(proc_arrow);
    });


    app.Run(true);

}

