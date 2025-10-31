
#include "JANA/JEventUnfolder.h"
#include <JANA/JApplicationFwd.h>
#include <catch.hpp>
#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JUnfoldArrow.h>
#include <JANA/Topology/JFoldArrow.h>
#include <cstdint>

#if JANA2_HAVE_PODIO
#include <PodioDatamodel/ExampleHitCollection.h>
#include <PodioDatamodel/EventInfoCollection.h>
#endif

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
    JEventQueue parent_queue {3, 1};
    JEventQueue child_queue {3, 1};

    auto ts1 = parent_pool.Pop(0);
    ts1->SetEventNumber(17);

    auto ts2 = parent_pool.Pop(0);
    ts2->SetEventNumber(28);

    parent_queue.Push(ts1, 0);
    parent_queue.Push(ts2, 0);

    TestUnfolder unfolder;
    JUnfoldArrow arrow("sut", &unfolder);
    arrow.attach(&parent_queue, JUnfoldArrow::PARENT_IN);
    arrow.attach(&child_pool, JUnfoldArrow::CHILD_IN);
    arrow.attach(&child_queue, JUnfoldArrow::CHILD_OUT);

    arrow.initialize();
    arrow.execute( 0); // First call to execute() picks up the parent and exits early
    auto result = arrow.execute( 0); // Second call to execute() picks up the child, calls Unfold(), and emits the newly parented child
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    REQUIRE(child_queue.GetSize(0) == 1);
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
    JEventQueue child_in(5, 1);
    JEventQueue child_out(5, 1);
    JEventQueue parent_out(5, 1);

    JFoldArrow arrow("sut", JEventLevel::Timeslice, JEventLevel::PhysicsEvent);
    arrow.attach(&child_in, JFoldArrow::CHILD_IN);
    arrow.attach(&child_out, JFoldArrow::CHILD_OUT);
    arrow.attach(&parent_out, JFoldArrow::PARENT_OUT);
    arrow.initialize();

    SECTION("One-to-one relationship between timeslices and events") {

        auto ts1 = parent_pool.Pop(0);
        ts1->SetEventNumber(17);
        REQUIRE(ts1->GetLevel() == JEventLevel::Timeslice);

        auto ts2 = parent_pool.Pop(0);
        ts2->SetEventNumber(28);

        auto evt1 = child_pool.Pop(0);
        evt1->SetEventNumber(111);

        auto evt2 = child_pool.Pop(0);
        evt2->SetEventNumber(112);


        evt1->SetParent(ts1);
        child_in.Push(evt1, 0);

        evt2->SetParent(ts2);
        child_in.Push(evt2, 0);
    
        arrow.execute(0);

        REQUIRE(child_in.GetSize(0) == 1);
        REQUIRE(child_out.GetSize(0) == 1);
        REQUIRE(parent_out.GetSize(0) == 1);

    }


    SECTION("One-to-two relationship between timeslices and events") {

        auto ts1 = parent_pool.Pop(0);
        ts1->SetEventNumber(17);
        REQUIRE(ts1->GetLevel() == JEventLevel::Timeslice);

        auto ts2 = parent_pool.Pop(0);
        ts2->SetEventNumber(28);

        auto evt1 = child_pool.Pop(0);
        evt1->SetEventNumber(111);

        auto evt2 = child_pool.Pop(0);
        evt2->SetEventNumber(112);

        auto evt3 = child_pool.Pop(0);
        evt3->SetEventNumber(113);

        auto evt4 = child_pool.Pop(0);
        evt4->SetEventNumber(114);


        evt1->SetParent(ts1);
        evt2->SetParent(ts1);
        
        evt3->SetParent(ts2);
        evt4->SetParent(ts2);
   
        child_in.Push(evt1, 0);
        child_in.Push(evt2, 0);
        child_in.Push(evt3, 0);
        child_in.Push(evt4, 0);

        arrow.execute(0);

        REQUIRE(child_in.GetSize(0) == 3);
        REQUIRE(child_out.GetSize(0) == 1);
        REQUIRE(parent_out.GetSize(0) == 0);

        arrow.execute(0);

        REQUIRE(child_in.GetSize(0) == 2);
        REQUIRE(child_out.GetSize(0) == 2);
        REQUIRE(parent_out.GetSize(0) == 1);

        arrow.execute(0);

        REQUIRE(child_in.GetSize(0) == 1);
        REQUIRE(child_out.GetSize(0) == 3);
        REQUIRE(parent_out.GetSize(0) == 1);

        arrow.execute(0);

        REQUIRE(child_in.GetSize(0) == 0);
        REQUIRE(child_out.GetSize(0) == 4);
        REQUIRE(parent_out.GetSize(0) == 2);
    }


}


class NoOpUnfolder : public JEventUnfolder {
#if JANA2_HAVE_PODIO
    PodioOutput<ExampleHit> m_hits_out {this}; 
    // We never insert these hits, and that should be fine 
    // because they should never get pushed to the frame
#endif

public:
    NoOpUnfolder() {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }
    Result Unfold(const JEvent&, JEvent&, int) {
        return Result::KeepChildNextParent;
    }
};

class PhysEvtProc : public JEventProcessor {
    int m_events_seen = 0;
public:
    PhysEvtProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent&) {
        m_events_seen += 1;
    }
    void Finish() {
        REQUIRE(m_events_seen == 0);
    }
};

class TimesliceProc : public JEventProcessor {
    int m_events_seen = 0;
public:
    TimesliceProc() {
        SetLevel(JEventLevel::Timeslice);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessSequential(const JEvent&) {
        m_events_seen += 1;
    }
    void Finish() {
        REQUIRE(m_events_seen == 10);
    }
};

TEST_CASE("NoOpUnfolder_Tests") {
    JApplication app;
    auto source = new JEventSource();
    source->SetLevel(JEventLevel::Timeslice);
    app.Add(source);
    app.Add(new NoOpUnfolder);
    app.Add(new PhysEvtProc);
    app.Add(new TimesliceProc);
    app.SetParameterValue("jana:nevents", 10);
    app.SetParameterValue("jana:loglevel", "debug");
    app.Run();
}


#if JANA2_HAVE_PODIO

/*
* This test case hopefully finds issues that the ePIC timeframe splitter will eventually encounter
*/

struct ePICSource : public JEventSource {
    std::vector<std::vector<std::tuple<int, int, double>>> datastream;
    size_t next_idx = 0;

    PodioOutput<EventInfo> info_out {this, "info"};
    VariadicPodioOutput<ExampleHit> hits_out {this, {"adet_hits", "bdet_hits"}};

    ePICSource() {
        SetTypeName("ePICSource");
        SetLevel(JEventLevel::Timeslice);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    Result Emit(JEvent&) override {
        if (next_idx == datastream.size()) {
            return Result::FailureFinished;
        }

        MutableEventInfo info;
        info.TimesliceNumber(next_idx);
        info_out()->push_back(info);

        const auto& timeslice = datastream.at(next_idx);
        for (auto& hit_data: timeslice) {
            MutableExampleHit hit;
            int det = std::get<0>(hit_data);
            int time = std::get<1>(hit_data);
            double energy = std::get<2>(hit_data);
            hit.time(time);
            hit.energy(energy);
            hits_out().at(det)->push_back(hit);
        }
        LOG << "Emitting timeslice " << next_idx << " containing " << timeslice.size() << " hits";
        next_idx += 1;

        return Result::Success;
    }
};

struct TimeAdjustment : public JFactory {
    VariadicPodioInput<ExampleHit> hits_in {this, VariadicInputOptions{.names={"adet_hits", "bdet_hits"}}};
    VariadicPodioOutput<ExampleHit> hits_out {this, {"adet_hits_adjusted", "bdet_hits_adjusted"}};

    TimeAdjustment() {
        SetLevel(JEventLevel::Timeslice);
    }

    void Process(const JEvent&) {
        size_t i=0;
        for (const auto& hit_coll : hits_in()) {
            for (const auto& raw_hit : *hit_coll) {
                auto adjusted_hit = raw_hit.clone();
                adjusted_hit.time(adjusted_hit.time() + 1);
                hits_out().at(i)->push_back(adjusted_hit);
            }
            i += 1;
        }
    }
};


struct Splitter : public JEventUnfolder {
    PodioInput<EventInfo> info_in {this, {.name="info"}};
    PodioOutput<EventInfo> info_out {this, "info"};
    VariadicPodioInput<ExampleHit> hits_in {this, VariadicInputOptions{.names={"adet_hits_adjusted", "bdet_hits_adjusted"}}};
    VariadicPodioOutput<ExampleHit> hits_out {this, {"adet_hits", "bdet_hits"}};

    uint64_t current_t = 0;
    uint64_t current_evt_nr = 0;

    Splitter() {
        info_out.SetSubsetCollection(true);
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }

    JEventUnfolder::Result Unfold(const JEvent&, JEvent& child, int) {

        REQUIRE(info_in()->size() == 1);

        LOG << "Unfolding timeslice " << info_in()->at(0).TimesliceNumber() << " into physics event " << current_evt_nr;

        for (; current_t < 10; current_t += 1) {

            bool hits_found = false;
            size_t current_coll_idx=0;
            for (const auto& hit_coll : hits_in()) {
                for (const auto& hit : *hit_coll) {
                    if (hit.time() == current_t) {
                        hits_found = true;
                        hits_out().at(current_coll_idx)->setSubsetCollection(true);
                        hits_out().at(current_coll_idx)->push_back(hit);
                    }
                }
                current_coll_idx += 1;
            }

            if (hits_found) {
                if (current_t == 9) {
                    // We've reached the last timestep in this timeslice
                    current_t = 0;
                    LOG << "Splitter: NextChildNextParent";
                    info_out()->push_back(info_in()->at(0));
                    child.SetEventNumber(current_evt_nr++);
                    return Result::NextChildNextParent;
                }
                else {
                    // Next call to Unfold() starts with the subsequent timestep
                    current_t += 1;
                    LOG << "Splitter: NextChildKeepParent";
                    info_out()->push_back(info_in()->at(0));
                    child.SetEventNumber(current_evt_nr++);
                    return Result::NextChildKeepParent;
                }
            }
        }
        // No (remaining) hits found in this timeslice for any timestep
        current_t = 0;
        LOG << "Splitter: KeepChildNextParent";
        return Result::KeepChildNextParent; 
    }
};


struct Checker : public JEventProcessor {
    PodioInput<EventInfo> info_in {this, {.name="info"}};
    VariadicPodioInput<ExampleHit> hits_in {this, VariadicInputOptions{.names={"adet_hits", "bdet_hits"}}};

    std::vector<int> expected_timeslice_nrs;
    std::vector<std::vector<std::tuple<int, int, double>>> expected_hits;
    size_t current_idx = 0;

    Checker() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        EnableOrdering();
    }
    void ProcessSequential(const JEvent& event) override {

        int ts_nr = info_in->at(0).TimesliceNumber();
        LOG << "Checking event " << event.GetEventNumber() << " from timeslice " << ts_nr;

        std::vector<std::tuple<int, int, double>> hits_found;

        size_t det_id = 0;
        for (const auto& coll : hits_in()) {
            for (const auto& hit : *coll) {
                hits_found.push_back({ det_id, hit.time(), hit.energy()});
            }
            det_id += 1;
        }

        REQUIRE(expected_timeslice_nrs.at(current_idx) == ts_nr);
        REQUIRE(expected_hits.at(current_idx).size() == hits_found.size());
        for (size_t i=0; i<hits_found.size(); ++i) {
            REQUIRE(std::get<0>(expected_hits.at(current_idx).at(i)) == std::get<0>(hits_found.at(i)));
            REQUIRE(std::get<1>(expected_hits.at(current_idx).at(i)) == std::get<1>(hits_found.at(i)));
            REQUIRE(std::get<2>(expected_hits.at(current_idx).at(i)) == std::get<2>(hits_found.at(i)));
        }

        current_idx += 1;
    }
};


TEST_CASE("ePIC_Timeframe_Splitting") {
    JApplication app;
    auto src = new ePICSource;
    auto checker = new Checker();

    app.Add(src);
    app.Add(new JFactoryGeneratorT<TimeAdjustment>);
    app.Add(new Splitter);
    app.Add(checker);

    const int DetA = 0;
    const int DetB = 1;

    SECTION("JustOne") {
        src->datastream = {
            {{DetA, 0, 22.2}},
            {{DetA, 0, 33.3}},
            {{DetA, 0, 44.4}}
        };
        checker->expected_timeslice_nrs = {0, 1, 2};
        checker->expected_hits = {
            {{DetA, 1, 22.2}},
            {{DetA, 1, 33.3}},
            {{DetA, 1, 44.4}}
        };
        app.Run();
    }
    SECTION("OneOrZero") {
        src->datastream = {
            {{DetA, 0, 22.2}},
            {},
            {{DetA, 0, 33.3}},
            {{DetA, 0, 44.4}}
        };
        checker->expected_timeslice_nrs = {0, 2, 3};
        checker->expected_hits = {
            {{DetA, 1, 22.2}},
            {{DetA, 1, 33.3}},
            {{DetA, 1, 44.4}}
        };
        app.Run();
    }
    SECTION("FlatMap") {
        src->datastream = {
            {{DetA, 0, 22.2}, {DetA, 1, 33.3}},
            {{DetA, 0, 44.4}, {DetA, 1, 55.5}, {DetB, 2, 66.6}},
            {{DetA, 0, 77.7}}
        };
        checker->expected_timeslice_nrs = {0,0,1,1,1,2};
        checker->expected_hits = {
            {{DetA, 1, 22.2}},
            {{DetA, 2, 33.3}},
            {{DetA, 1, 44.4}},
            {{DetA, 2, 55.5}},
            {{DetB, 3, 66.6}},
            {{DetA, 1, 77.7}}
        };
        app.Run();
    }
    SECTION("UnFlatMap") {
        src->datastream = {
            {{DetA, 0, 22.2}, {DetA, 1, 33.3}, {DetA, 1, 19}, {DetB, 1, 22}},
            {{DetA, 0, 44.4}, {DetA, 1, 55.5}, {DetB, 2, 66.6}},
            {{DetA, 0, 77.7}, {DetA, 0, 88.8}, {DetA, 0, 99.9}}
        };
        checker->expected_timeslice_nrs = {0,0,1,1,1,2};
        checker->expected_hits = {
            {{DetA, 1, 22.2}},
            {{DetA, 2, 33.3}, {DetA, 2, 19}, {DetB, 2, 22}},
            {{DetA, 1, 44.4}},
            {{DetA, 2, 55.5}},
            {{DetB, 3, 66.6}},
            {{DetA, 1, 77.7}, {DetA, 1, 88.8}, {DetA, 1, 99.9}}
        };
        app.Run();
    }
}

#endif // JANA2_HAVE_PODIO

} // namespace unfoldtests
} // namespace jana




