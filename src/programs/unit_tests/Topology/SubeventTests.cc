
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <catch.hpp>

#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventMapArrow.h>
#include <JANA/Topology/JSubeventArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>


struct MyInput : public JObject {
    int x;
    float y;
    MyInput(int x, float y) : x(x), y(y) {}
};

struct MyOutput : public JObject {
    float z;
    explicit MyOutput(float z) : z(z) {}
};

struct MyProcessor : public JSubeventProcessor<MyInput, MyOutput> {
    MyProcessor() {
        inputTag = "";
        outputTag = "subeventted";
    }
    MyOutput* ProcessSubevent(MyInput* input) override {
        return new MyOutput(input->y + (float) input->x);
    }
};

TEST_CASE("Create subevent processor") {

    MyProcessor processor;
    MyInput input(22, 7.6);
    MyOutput* output = processor.ProcessSubevent(&input);
    REQUIRE(output->z == 29.6f);
}


TEST_CASE("Basic subevent arrow functionality") {

    MyProcessor processor;
    JMailbox<JEvent*> events_in;
    JMailbox<JEvent*> events_out;
    JMailbox<SubeventWrapper<MyInput>> subevents_in;
    JMailbox<SubeventWrapper<MyOutput>> subevents_out;

    auto split_arrow = new JSplitArrow<MyInput, MyOutput>("split", &processor, &events_in, &subevents_in);
    auto subprocess_arrow = new JSubeventArrow<MyInput, MyOutput>("subprocess", &processor, &subevents_in, &subevents_out);
    auto merge_arrow = new JMergeArrow<MyInput, MyOutput>("merge", &processor, &subevents_out, &events_out);

    SECTION("No-op execute subevent arrows") {
        JArrowMetrics m;
        split_arrow->execute(m, 0);
        merge_arrow->execute(m, 0);
        subprocess_arrow->execute(m, 0);
    }

    struct SimpleSource : public JEventSource {
        SimpleSource() {
            SetCallbackStyle(CallbackStyle::ExpertMode);
        }
        Result Emit(JEvent& event) override {
            if (GetEmittedEventCount() == 10) return Result::FailureFinished;
            std::vector<MyInput*> inputs;
            inputs.push_back(new MyInput(22,3.6));
            inputs.push_back(new MyInput(23,3.5));
            inputs.push_back(new MyInput(24,3.4));
            inputs.push_back(new MyInput(25,3.3));
            event.Insert(inputs);
            return Result::Success;
        }
    };

    struct SimpleProcessor : public JEventProcessor {
        SimpleProcessor() {
            SetCallbackStyle(CallbackStyle::ExpertMode);
        }
        void Process(const JEvent& event) {
            auto outputs = event.Get<MyOutput>();
            REQUIRE(outputs.size() == 4);
            REQUIRE(outputs[0]->z == 25.6f);
            REQUIRE(outputs[1]->z == 26.5f);
            REQUIRE(outputs[2]->z == 27.4f);
            REQUIRE(outputs[3]->z == 28.3f);
        }
    };

    SECTION("Execute subevent arrows end-to-end") {

        JApplication app;
        app.SetTimeoutEnabled(false);
        app.SetTicker(false);

        auto topology = app.GetService<JTopologyBuilder>();
        topology->set_configure_fn([&](JTopologyBuilder& topology) {

            auto source_arrow = new JEventSourceArrow("simpleSource", {new SimpleSource});
            source_arrow->attach(topology.event_pool, JEventSourceArrow::EVENT_IN);
            source_arrow->attach(&events_in, JEventSourceArrow::EVENT_OUT);

            auto proc_arrow = new JEventMapArrow("simpleProcessor");
            proc_arrow->attach(&events_out, 0);
            proc_arrow->attach(topology.event_pool, 1);
            proc_arrow->add_processor(new SimpleProcessor);

            topology.arrows.push_back(source_arrow);
            topology.arrows.push_back(split_arrow);
            topology.arrows.push_back(subprocess_arrow);
            topology.arrows.push_back(merge_arrow);
            topology.arrows.push_back(proc_arrow);

            source_arrow->attach(split_arrow);
            split_arrow->attach(subprocess_arrow);
            subprocess_arrow->attach(merge_arrow);
            merge_arrow->attach(proc_arrow);
        });

        app.Run(true);
    }


}


