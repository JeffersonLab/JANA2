
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <catch.hpp>

#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Topology/JEventProcessorArrow.h>
#include <JANA/Topology/JSubeventArrow.h>
#include <JANA/Topology/JArrowTopology.h>
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

#if 0
TEST_CASE("Simplest working SubeventMailbox") {

    std::vector<SubeventWrapper<MyOutput>> unmerged;
    auto event1 = std::make_shared<JEvent>();
    auto event2 = std::make_shared<JEvent>();

    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(2), 1, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(4), 2, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(6), 3, 5));

    JSubeventMailbox<MyOutput> mailbox;
    mailbox.push(unmerged);

    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(8), 4, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(10), 5, 5));
    mailbox.push(unmerged);

    std::vector<std::shared_ptr<JEvent>*> merged;
    JSubeventMailbox<MyOutput>::Status result = mailbox.pop(merged, 1);

    REQUIRE(result == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    auto items_in_event = (*(merged[0]))->Get<MyOutput>();
    REQUIRE(items_in_event.size() == 5);
}


TEST_CASE("SubeventMailbox with two overlapping events") {

    JSubeventMailbox<MyOutput> mailbox;
    std::vector<SubeventWrapper<MyOutput>> unmerged;
    std::vector<std::shared_ptr<JEvent>*> merged;

    auto event1 = std::make_shared<JEvent>();
    auto event2 = std::make_shared<JEvent>();

    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(2), 1, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(4), 2, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(6), 3, 5));

    mailbox.push(unmerged);

    // We don't have a complete event merged yet
    JSubeventMailbox<MyOutput>::Status result = mailbox.pop(merged, 1);
    REQUIRE(result == JSubeventMailbox<MyOutput>::Status::Empty);


    // Now we mix in some subevents from event 2
    unmerged.push_back(SubeventWrapper<MyOutput>(&event2, new MyOutput(1), 1, 4));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event2, new MyOutput(3), 2, 4));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event2, new MyOutput(5), 3, 4));

    // Still not able to pop anything because neither of the events are complete
    JSubeventMailbox<MyOutput>::Status result1 = mailbox.pop(merged, 1);
    REQUIRE(result1 == JSubeventMailbox<MyOutput>::Status::Empty);

    // Now we receive the rest of the subevents from event 1
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(8), 4, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(&event1, new MyOutput(10), 5, 5));
    mailbox.push(unmerged);

    // We were able to get event1 out, but not event 2
    JSubeventMailbox<MyOutput>::Status result2 = mailbox.pop(merged, 2);
    REQUIRE(result2 == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    REQUIRE(merged[0] == &event1);
    merged.clear();

    // Now we add the remaining subevents from event 2
    unmerged.push_back(SubeventWrapper<MyOutput>(&event2, new MyOutput(7), 4, 4));
    mailbox.push(unmerged);

    // Now we can pop event2
    JSubeventMailbox<MyOutput>::Status result3 = mailbox.pop(merged, 2);
    REQUIRE(result3 == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    REQUIRE(merged[0] == &event2);
    auto items_in_event = (*(merged[0]))->Get<MyOutput>();
    REQUIRE(items_in_event.size() == 4);
}
#endif

TEST_CASE("Basic subevent arrow functionality") {

    MyProcessor processor;
    JMailbox<std::shared_ptr<JEvent>*> events_in;
    JMailbox<std::shared_ptr<JEvent>*> events_out;
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
            if (GetEventCount() == 10) return Result::FailureFinished;
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

    SECTION("Execute subevent arrows end-to-end using same example as in JSubeventMailbox") {

        JApplication app;
        app.SetParameterValue("log:info", "JWorker,JScheduler,JArrow,JArrowProcessingController,JEventProcessorArrow");
        app.SetTimeoutEnabled(false);
        app.SetTicker(false);

        auto topology = app.GetService<JTopologyBuilder>()->create_empty();
        auto source_arrow = new JEventSourceArrow("simpleSource",
                                                  {new SimpleSource},
                                                  &events_in,
                                                  topology->event_pool);
        auto proc_arrow = new JEventProcessorArrow("simpleProcessor", &events_out, nullptr, topology->event_pool);
        proc_arrow->add_processor(new SimpleProcessor);

        topology->arrows.push_back(source_arrow);
        topology->arrows.push_back(split_arrow);
        topology->arrows.push_back(subprocess_arrow);
        topology->arrows.push_back(merge_arrow);
        topology->arrows.push_back(proc_arrow);
        source_arrow->attach(split_arrow);
        split_arrow->attach(subprocess_arrow);
        subprocess_arrow->attach(merge_arrow);
        merge_arrow->attach(proc_arrow);

        app.Run(true);
    }


}


