
#include <catch.hpp>
#include <JANA/Topology/JUnfoldArrow.h>
#include <JANA/Topology/JFoldArrow.h>

namespace jana {
namespace unfoldtests {


struct TestUnfolder : public JEventUnfolder {
    mutable std::vector<int> preprocessed_event_nrs;
    mutable std::vector<JEventLevel> preprocessed_event_levels;
    std::vector<int> unfolded_parent_nrs;
    std::vector<JEventLevel> unfolded_parent_levels;
    std::vector<int> unfolded_child_nrs;
    std::vector<JEventLevel> unfolded_child_levels;

    TestUnfolder() {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }

    void Preprocess(const JEvent& parent) const override {
        LOG << "Preprocessing " << parent.GetLevel() << " event " << parent.GetEventNumber() << LOG_END;
        preprocessed_event_nrs.push_back(parent.GetEventNumber());
        preprocessed_event_levels.push_back(parent.GetLevel());
    }

    Result Unfold(const JEvent& parent, JEvent& child, int iter) override {
        auto child_nr = iter + 100 + parent.GetEventNumber();
        unfolded_parent_nrs.push_back(parent.GetEventNumber());
        unfolded_parent_levels.push_back(parent.GetLevel());
        unfolded_child_nrs.push_back(child_nr);
        unfolded_child_levels.push_back(child.GetLevel());
        child.SetEventNumber(child_nr);
        LOG << "Unfolding " << parent.GetLevel() << " event " << parent.GetEventNumber() << " into " << child.GetLevel() << " " << child_nr << "; iter=" << iter << LOG_END;
        return (iter == 2 ? Result::NextChildNextParent : Result::NextChildKeepParent);
    }
};

TEST_CASE("UnfoldTests_Basic") {

    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();

    JEventPool parent_pool {jcm, 5, 1, JEventLevel::Timeslice};
    JEventPool child_pool {jcm, 5, 1, JEventLevel::PhysicsEvent};
    JMailbox<JEvent*> parent_queue {3}; // size
    JMailbox<JEvent*> child_queue {3};

    auto ts1 = parent_pool.get();
    ts1->SetEventNumber(17);

    auto ts2 = parent_pool.get();
    ts2->SetEventNumber(28);

    parent_queue.try_push(&ts1, 1);
    parent_queue.try_push(&ts2, 1);

    TestUnfolder unfolder;
    JUnfoldArrow arrow("sut", &unfolder);
    arrow.attach_parent_in(&parent_queue);
    arrow.attach_child_in(&child_pool);
    arrow.attach_child_out(&child_queue);

    JArrowMetrics m;
    arrow.initialize();
    arrow.execute(m, 0); // First call to execute() picks up the parent and exits early
    arrow.execute(m, 0); // Second call to execute() picks up the child, calls Unfold(), and emits the newly parented child
    REQUIRE(m.get_last_status() == JArrowMetrics::Status::KeepGoing);
    REQUIRE(child_queue.size() == 1);
    REQUIRE(unfolder.preprocessed_event_nrs.size() == 0);
    REQUIRE(unfolder.unfolded_parent_nrs.size() == 1);
    REQUIRE(unfolder.unfolded_parent_nrs[0] == 17);
    REQUIRE(unfolder.unfolded_parent_levels[0] == JEventLevel::Timeslice);
    REQUIRE(unfolder.unfolded_child_nrs.size() == 1);
    REQUIRE(unfolder.unfolded_child_nrs[0] == 117);
    REQUIRE(unfolder.unfolded_child_levels[0] == JEventLevel::PhysicsEvent);

}

TEST_CASE("FoldArrowTests") {

    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();
    

    // We only use these to obtain preconfigured JEvents
    JEventPool parent_pool {jcm, 5, 1, JEventLevel::Timeslice};
    JEventPool child_pool {jcm, 5, 1, JEventLevel::PhysicsEvent};

    // We set up our test cases by putting events on these queues
    JMailbox<JEvent*> child_in;
    JMailbox<JEvent*> child_out;
    JMailbox<JEvent*> parent_out;

    JFoldArrow arrow("sut", JEventLevel::Timeslice, JEventLevel::PhysicsEvent);
    arrow.attach_child_in(&child_in);
    arrow.attach_child_out(&child_out);
    arrow.attach_parent_out(&parent_out);
    JArrowMetrics metrics;
    arrow.initialize();

    SECTION("One-to-one relationship between timeslices and events") {

        auto ts1 = parent_pool.get();
        ts1->SetEventNumber(17);
        REQUIRE(ts1->GetLevel() == JEventLevel::Timeslice);

        auto ts2 = parent_pool.get();
        ts2->SetEventNumber(28);

        auto evt1 = child_pool.get();
        evt1->SetEventNumber(111);

        auto evt2 = child_pool.get();
        evt2->SetEventNumber(112);


        evt1->SetParent(ts1);
        ts1->Release(); // One-to-one
        child_in.try_push(&evt1, 1, 0);

        evt2->SetParent(ts2);
        ts2->Release(); // One-to-one
        child_in.try_push(&evt2, 1, 0);
    
        arrow.execute(metrics, 0);

        REQUIRE(child_in.size() == 1);
        REQUIRE(child_out.size() == 1);
        REQUIRE(parent_out.size() == 1);

    }


    SECTION("One-to-two relationship between timeslices and events") {

        auto ts1 = parent_pool.get();
        ts1->SetEventNumber(17);
        REQUIRE(ts1->GetLevel() == JEventLevel::Timeslice);

        auto ts2 = parent_pool.get();
        ts2->SetEventNumber(28);

        auto evt1 = child_pool.get();
        evt1->SetEventNumber(111);

        auto evt2 = child_pool.get();
        evt2->SetEventNumber(112);

        auto evt3 = child_pool.get();
        evt3->SetEventNumber(113);

        auto evt4 = child_pool.get();
        evt4->SetEventNumber(114);


        evt1->SetParent(ts1);
        evt2->SetParent(ts1);
        ts1->Release(); // One-to-two
        
        evt3->SetParent(ts2);
        evt4->SetParent(ts2);
        ts2->Release(); // One-to-two
   
        child_in.try_push(&evt1, 1, 0);
        child_in.try_push(&evt2, 1, 0);
        child_in.try_push(&evt3, 1, 0);
        child_in.try_push(&evt4, 1, 0);

        arrow.execute(metrics, 0);

        REQUIRE(child_in.size() == 3);
        REQUIRE(child_out.size() == 1);
        REQUIRE(parent_out.size() == 0);

        arrow.execute(metrics, 0);

        REQUIRE(child_in.size() == 2);
        REQUIRE(child_out.size() == 2);
        REQUIRE(parent_out.size() == 1);

        arrow.execute(metrics, 0);

        REQUIRE(child_in.size() == 1);
        REQUIRE(child_out.size() == 3);
        REQUIRE(parent_out.size() == 1);

        arrow.execute(metrics, 0);

        REQUIRE(child_in.size() == 0);
        REQUIRE(child_out.size() == 4);
        REQUIRE(parent_out.size() == 2);
    }


}

    
} // namespace arrowtests
} // namespace jana




