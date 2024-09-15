
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessorSequential.h>
#include <JANA/JEventProcessorSequentialRoot.h>

namespace jeventprocessorsequentialtests {
// If you reuse type names in different Catch tests (e.g. DummyFactory),
// the linker will cheerfully not notice and you will get VERY weird errors.
// Hence, we protect each Catch test with its own namespace.


struct MyRootProcessor : public JEventProcessorSequentialRoot {
    std::vector<std::pair<int, int>> access_log;
    std::mutex access_log_mutex;

    void InitWithGlobalRootLock() override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(42, i));
        }
    }

    void ProcessSequential(const std::shared_ptr<const JEvent> &event) override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40 - i * 10));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(event->GetEventNumber(), i));
        }
    }

    void FinishWithGlobalRootLock() override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(49, i));
        }
    }
};

TEST_CASE("JEventProcessorSequentialRootTests") {

    JApplication app;
    app.Add(new JEventSource());
    app.SetParameterValue("nthreads", 4);
    app.SetParameterValue("jana:nevents", 4);
    app.SetParameterValue("jana:event_source_chunksize", 1);
    app.SetParameterValue("jana:event_processor_chunksize", 1);
    app.SetParameterValue("jana:loglevel", "warn");
    auto proc = new MyRootProcessor;
    app.Add(proc);
    app.Run(true);

    auto log = proc->access_log;
    // for (auto pair: log) {
    //     jout << pair.first << ", " << pair.second << jendl;
    // }
    // If there are no race conditions, we expect the access log to contain:
    // 1. Init
    // 2. 4 events with 10 iters apiece
    //    - The iterations within each event are in the correct order
    //    - Events are NOT INTERLEAVED
    //    - Events might NOT be visited in order of event number
    //    - All events must be visited
    // 3. Finish
    REQUIRE(log.size() == 60);
    // Check that Init is called correctly
    for (int i = 0; i < 10; ++i) {
        REQUIRE(log[i].first == 42);
        REQUIRE(log[i].second == i);
    }
    // Check that Finish is called correctly
    for (int i = 0; i < 10; ++i) {
        REQUIRE(log[50 + i].first == 49);
        REQUIRE(log[50 + i].second == i);
    }
    bool event_processed[4] = {false, false, false, false};

    // Check that each event is processed correctly
    for (int event = 0; event < 4; ++event) {
        int event_nr = log[10 + event * 10].first;
        REQUIRE(event_processed[event_nr] == false);
        event_processed[event_nr] = true;
        for (int i = 0; i < 10; ++i) {
            auto pair = log[10 + event * 10 + i];
            REQUIRE(pair.first == event_nr);
            REQUIRE(pair.second == i);
        }
    }

    // All events were processed
    for (int event = 0; event < 4; ++event) {
        REQUIRE(event_processed[event] == true);
    }
}

struct MySeqProcessor : public JEventProcessorSequential {
    std::vector<std::pair<int, int>> access_log;
    std::mutex access_log_mutex;

    void Init() override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(42, i));
        }
    }

    void ProcessSequential(const std::shared_ptr<const JEvent> &event) override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40 - i * 10));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(event->GetEventNumber(), i));
        }
    }

    void Finish() override {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            std::lock_guard<std::mutex> lock(access_log_mutex);
            access_log.emplace_back(std::make_pair(49, i));
        }
    }
};


TEST_CASE("JEventProcessorSequentialTests") {

    JApplication app;
    app.Add(new JEventSource());
    app.SetParameterValue("nthreads", 4);
    app.SetParameterValue("jana:nevents", 4);
    app.SetParameterValue("jana:event_source_chunksize", 1);
    app.SetParameterValue("jana:event_processor_chunksize", 1);
    app.SetParameterValue("jana:loglevel", "warn");
    auto proc = new MySeqProcessor;
    app.Add(proc);
    app.Run(true);

    auto log = proc->access_log;
    // for (auto pair: log) {
    //     jout << pair.first << ", " << pair.second << jendl;
    // }
    // If there are no race conditions, we expect the access log to contain:
    // 1. Init
    // 2. 4 events with 10 iters apiece
    //    - The iterations within each event are in the correct order
    //    - Events are NOT INTERLEAVED
    //    - Events might NOT be visited in order of event number
    //    - All events must be visited
    // 3. Finish
    REQUIRE(log.size() == 60);
    // Check that Init is called correctly
    for (int i = 0; i < 10; ++i) {
        REQUIRE(log[i].first == 42);
        REQUIRE(log[i].second == i);
    }
    // Check that Finish is called correctly
    for (int i = 0; i < 10; ++i) {
        REQUIRE(log[50 + i].first == 49);
        REQUIRE(log[50 + i].second == i);
    }
    bool event_processed[4] = {false, false, false, false};

    // Check that each event is processed correctly
    for (int event = 0; event < 4; ++event) {
        int event_nr = log[10 + event * 10].first;
        REQUIRE(event_processed[event_nr] == false);
        event_processed[event_nr] = true;
        for (int i = 0; i < 10; ++i) {
            auto pair = log[10 + event * 10 + i];
            REQUIRE(pair.first == event_nr);
            REQUIRE(pair.second == i);
        }
    }
}

} // namespace
