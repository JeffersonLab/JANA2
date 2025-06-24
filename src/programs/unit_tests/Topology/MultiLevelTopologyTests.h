#pragma once
#include <JANA/JApplication.h>
#include <JANA/JObject.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <catch.hpp>

namespace jana {
namespace timeslice_tests {


struct MyHit : public JObject {
    int hit_id;
    int energy, x, y;
};

struct MyCluster : public JObject {
    int cluster_id;
    int energy, x, y;
    std::vector<int> hits;
};


struct MyTimesliceSource : public JEventSource {

    MyTimesliceSource() { 
        SetLevel(JEventLevel::Timeslice);
    }

    void Open() override { }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        // TODO: Insert something
        LOG_INFO(GetLogger()) << "MyTimesliceSource: Emitting " << event->GetEventNumber() << LOG_END;
    }
};


struct MyTimesliceUnfolder : public JEventUnfolder {

    std::atomic_int init_called_count {0};
    mutable std::atomic_int preprocess_called_count {0};
    std::atomic_int unfold_called_count {0};
    std::atomic_int finish_called_count {0};

    MyTimesliceUnfolder() {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }

    virtual void Init() {
        init_called_count++;
    };

    virtual void Preprocess(const JEvent& /*parent*/) const {
        preprocess_called_count++;
        // TODO: Are we going to need an omni unfolder?
        // TODO: Call protocluster factory
    };

    virtual Result Unfold(const JEvent& parent, JEvent& child, int item) {
        child.SetEventNumber(parent.GetEventNumber()*10 + item);
        LOG << "Unfolding parent=" << parent.GetEventNumber() << ", child=" << child.GetEventNumber() << ", item=" << item << LOG_END;
        unfold_called_count++;
        // TODO: 

        if (item == 3) {
            LOG_INFO(GetLogger()) << "Unfold found item 3, finishing join" << LOG_END;
            // TODO: Insert protocluster into child
            return Result::NextChildNextParent;
        }
        return Result::NextChildKeepParent;
    }

    virtual void Finish() {
        finish_called_count++;
    };

};

struct MyEventProcessor : public JEventProcessor {

    std::atomic_int init_called_count {0};
    std::atomic_int process_called_count {0};
    std::atomic_int finish_called_count {0};

    MyEventProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Init() override {
        init_called_count++;
    }

    void ProcessSequential(const JEvent& event) override {
        process_called_count++;
        LOG_INFO(GetLogger()) << "MyEventProcessor: Processing " << event.GetEventNumber() << LOG_END;
        auto clusters = event.Get<MyCluster>("evt");
        // TODO: Validate that the clusters make sense
        REQUIRE(init_called_count == 1);
        REQUIRE(finish_called_count == 0);
    }

    void Finish() override {
        finish_called_count += 1;
    }

};


struct MyProtoClusterFactory : public JFactoryT<MyCluster> {

    int init_call_count = 0;
    int change_run_call_count = 0;
    int process_call_count = 0;

    MyProtoClusterFactory() {
        SetLevel(JEventLevel::Timeslice);
        SetTag("ts");
    }

    void Init() override {
        ++init_call_count;
    }

    void ChangeRun(const std::shared_ptr<const JEvent>&) override {
        ++change_run_call_count;
    }

    void Process(const std::shared_ptr<const JEvent>&) override {
        ++process_call_count;
        Insert(new MyCluster);
        // TODO: Obtain timeslice-level hits
    }
};


struct MyClusterFactory : public JFactoryT<MyCluster> {

    int init_call_count = 0;
    int change_run_call_count = 0;
    int process_call_count = 0;

    MyClusterFactory() {
        SetLevel(JEventLevel::PhysicsEvent);
        SetTag("evt");
    }

    void Init() override {
        ++init_call_count;
    }

    void ChangeRun(const std::shared_ptr<const JEvent>&) override {
        ++change_run_call_count;
    }

    void Process(const std::shared_ptr<const JEvent>&) override {
        ++process_call_count;
        Insert(new MyCluster);
        // TODO: Obtain timeslice-level protoclusters
    }
};

} // namespace timeslice_tests
} // namespace jana
