#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>

#include <catch.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace jana::integration_tests::hierarchical_ownership {

static constexpr uint64_t CANARY = 0xABCDEF0123456789ULL;

struct BlockData {                      // inserted into the parent (timeslice) by the source
    std::vector<uint64_t> samples;
    uint64_t canary = CANARY;
};

struct FrameRef {                       // child payload: pointer INTO the parent's BlockData,
    const BlockData* block;             // valid per the parent-lifetime guarantee
    int frame_index;
};

struct TimesliceSource : JEventSource {
    TimesliceSource() {
        SetLevel(JEventLevel::Timeslice);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent& event) override {
        auto* block = new BlockData;
        block->samples.assign(100000, event.GetEventNumber());
        event.Insert(block);
        return Result::Success;
    }
};

struct FrameUnfolder : JEventUnfolder {
    FrameUnfolder() {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }
    Result Unfold(const JEvent& parent, JEvent& child, int child_idx) override {
        child.Insert(new FrameRef{parent.GetSingle<BlockData>(), child_idx});
        // Three children per timeslice; the last one takes the NextChildNextParent
        // branch, which pushes the parent to its pool in the same firing.
        return child_idx == 2 ? Result::NextChildNextParent : Result::NextChildKeepParent;
    }
};

struct FrameProcessor : JEventProcessor {
    FrameProcessor() { SetCallbackStyle(CallbackStyle::ExpertMode); }
    void ProcessSequential(const JEvent& event) override {
        const auto* ref = event.GetSingle<FrameRef>();
        REQUIRE(ref->block->canary == CANARY);
    }
};

TEST_CASE("ParentEventOutlivesChildren") {

    JApplication app;
    app.SetParameterValue("jana:nevents", 10);
    app.SetParameterValue("nthreads", 1);
    app.Add(new TimesliceSource);
    app.Add(new FrameUnfolder);
    app.Add(new FrameProcessor);
    app.Run();
}

} // namespace
