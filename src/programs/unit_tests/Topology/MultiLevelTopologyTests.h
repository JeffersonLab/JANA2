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

namespace multilevel_source_tests {

struct MyCalibs {int x=0; };
struct MyControls {int x=0; };
struct MyHits {int x=0; };

struct MyMultilevelSource : public JEventSource {

    std::vector<std::pair<JEventLevel, int>> data_stream;
    size_t data_stream_index = 0;

    Output<MyCalibs> m_calibs_out {this};
    Output<MyControls> m_controls_out {this};
    Output<MyHits> m_hits_out {this};

    MyMultilevelSource() {
        SetEventLevels({JEventLevel::Run, JEventLevel::SlowControls, JEventLevel::PhysicsEvent});

        m_calibs_out.SetLevel(JEventLevel::Run);
        m_controls_out.SetLevel(JEventLevel::SlowControls);
        m_hits_out.SetLevel(JEventLevel::PhysicsEvent);
    }

    Result Emit(JEvent& event) override {
        auto container_level = event.GetLevel();
        auto data_level = data_stream[data_stream_index].first;

        if (container_level != data_level) {
            SetNextEventLevel(data_level);
            return JEventSource::Result::FailureLevelChange;
        }

        if (data_level == JEventLevel::PhysicsEvent) {
            m_hits_out().push_back(new MyHits {data_stream[data_stream_index].second});
        }
        else if (data_level == JEventLevel::SlowControls) {
            m_controls_out().push_back(new MyControls {data_stream[data_stream_index].second});
        }
        else if (data_level == JEventLevel::Run) {
            m_calibs_out().push_back(new MyCalibs {data_stream[data_stream_index].second});
        }

        data_stream_index += 1;
        return Result::Success;
    }
};

struct MyMultilevelProcessor : public JEventProcessor {

    std::vector<std::tuple<int, int, int>> expected_data_stream;
    std::vector<std::tuple<int, int, int>> actual_data_stream;

    Input<MyCalibs> m_calibs_in {this};
    Input<MyControls> m_controls_in {this};
    Input<MyHits> m_hits_in {this};

    MyMultilevelProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        m_calibs_in.SetLevel(JEventLevel::Run);
        m_controls_in.SetLevel(JEventLevel::SlowControls);
        m_hits_in.SetLevel(JEventLevel::PhysicsEvent);
    }

    void ProcessSequential(const JEvent&) override {
        actual_data_stream.push_back({m_calibs_in->at(0)->x, m_controls_in->at(0)->x, m_hits_in->at(0)->x});
    }

    void Finish() override {
        REQUIRE(expected_data_stream == actual_data_stream);
    }

};


} //namespace multilevel_source_tests
} // namespace jana
