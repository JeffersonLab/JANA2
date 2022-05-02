
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <catch.hpp>

#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/Engine/JSubeventMailbox.h>
#include <JANA/Engine/JSubeventArrow.h>


struct MyInput : public JObject {
    int x;
    float y;
};

struct MyOutput : public JObject {
    float z;
    MyOutput(float z) : z(z) {}
};

struct MyProcessor : public JSubeventProcessor<MyInput, MyOutput> {
    MyProcessor() {
        inputTag = "";
        outputTag = "subeventted";
    }
    MyOutput* ProcessSubevent(MyInput* input) {
        return new MyOutput(input->y + input->x);
    }
};

TEST_CASE("Create subevent processor") {

    MyProcessor processor;
    MyInput input;
    input.x = 22;
    input.y = 7.6;
    MyOutput* output = processor.ProcessSubevent(&input);
    REQUIRE(output->z == 29.6f);
}


TEST_CASE("Simplest working SubeventMailbox") {

    std::vector<SubeventWrapper<MyOutput>> unmerged;
    auto event1 = std::make_shared<JEvent>();
    auto event2 = std::make_shared<JEvent>();

    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(2), 1, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(4), 2, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(6), 3, 5));

    JSubeventMailbox<MyOutput> mailbox;
    mailbox.set_active(true);
    mailbox.push(unmerged);

    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(8), 4, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(10), 5, 5));
    mailbox.push(unmerged);

    std::vector<std::shared_ptr<JEvent>> merged;
    JSubeventMailbox<MyOutput>::Status result = mailbox.pop(merged, 1);

    REQUIRE(result == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    auto items_in_event = merged[0]->Get<MyOutput>();
    REQUIRE(items_in_event.size() == 5);
}


TEST_CASE("SubeventMailbox with two overlapping events") {

    JSubeventMailbox<MyOutput> mailbox;
    mailbox.set_active(true);
    std::vector<SubeventWrapper<MyOutput>> unmerged;
    std::vector<std::shared_ptr<JEvent>> merged;

    auto event1 = std::make_shared<JEvent>();
    auto event2 = std::make_shared<JEvent>();

    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(2), 1, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(4), 2, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(6), 3, 5));

    mailbox.push(unmerged);

    // We don't have a complete event merged yet
    JSubeventMailbox<MyOutput>::Status result = mailbox.pop(merged, 1);
    REQUIRE(result == JSubeventMailbox<MyOutput>::Status::Empty);


    // Now we mix in some subevents from event 2
    unmerged.push_back(SubeventWrapper<MyOutput>(event2, new MyOutput(1), 1, 4));
    unmerged.push_back(SubeventWrapper<MyOutput>(event2, new MyOutput(3), 2, 4));
    unmerged.push_back(SubeventWrapper<MyOutput>(event2, new MyOutput(5), 3, 4));

    // Still not able to pop anything because neither of the events are complete
    JSubeventMailbox<MyOutput>::Status result1 = mailbox.pop(merged, 1);
    REQUIRE(result1 == JSubeventMailbox<MyOutput>::Status::Empty);

    // Now we receive the rest of the subevents from event 1
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(8), 4, 5));
    unmerged.push_back(SubeventWrapper<MyOutput>(event1, new MyOutput(10), 5, 5));
    mailbox.push(unmerged);

    // We were able to get event1 out, but not event 2
    JSubeventMailbox<MyOutput>::Status result2 = mailbox.pop(merged, 2);
    REQUIRE(result2 == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    REQUIRE(merged[0] == event1);
    merged.clear();

    // Now we add the remaining subevents from event 2
    unmerged.push_back(SubeventWrapper<MyOutput>(event2, new MyOutput(7), 4, 4));
    mailbox.push(unmerged);

    // Now we can pop event2
    JSubeventMailbox<MyOutput>::Status result3 = mailbox.pop(merged, 2);
    REQUIRE(result3 == JSubeventMailbox<MyOutput>::Status::Empty);
    REQUIRE(merged.size() == 1);
    REQUIRE(merged[0] == event2);
    auto items_in_event = merged[0]->Get<MyOutput>();
    REQUIRE(items_in_event.size() == 4);
}


TEST_CASE("Basic subevent arrow functionality") {

    MyProcessor processor;
    JMailbox<std::shared_ptr<JEvent>> events_in;
    JMailbox<std::shared_ptr<JEvent>> events_out;
    JMailbox<SubeventWrapper<MyInput>> subevents_in;
    JMailbox<SubeventWrapper<MyOutput>> subevents_out;

    JSplitArrow<MyInput, MyOutput> split_arrow("split", &processor, &events_in, &subevents_in);
    JSubeventArrow<MyInput, MyOutput> subprocess_arrow("subprocess", &processor, &subevents_in, &subevents_out);
    JMergeArrow<MyInput, MyOutput> merge_arrow("merge", &processor, &subevents_out, &events_out);

    SECTION("Execute subevent arrows") {
        JArrowMetrics m;
        split_arrow.execute(m, 0);
        merge_arrow.execute(m, 0);
        subprocess_arrow.execute(m, 0);
    }

}


