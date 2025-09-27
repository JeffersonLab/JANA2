
#include "JANA/JApplicationFwd.h"
#include "JANA/JEventSource.h"
#include "JANA/JLogger.h"
#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JEventProcessor.h>
#include <chrono>
#include <thread>


namespace jana::engine::ordering_tests {

struct MyData {int x;};

class MyFac : public JFactory {

    Output<MyData> m_data_out {this};
    int m_delays_s[4] = {2, 0, 5, 1};

public:
    MyFac() {
        SetPrefix("myfac");
        SetTypeName("MyFac");
    }
    void Process(const JEvent& event) override {
        auto event_nr = event.GetEventNumber();
        auto delay = m_delays_s[event_nr % 4];

        LOG_DEBUG(GetLogger()) << "Delaying event " << event_nr << " with index " << event.GetEventIndex() << " for " << delay << " seconds";
        std::this_thread::sleep_for(std::chrono::seconds(delay));
        LOG_DEBUG(GetLogger()) << "Finished Delaying event " << event_nr;
        m_data_out().push_back(new MyData{.x=delay});
    }
};

class MyProc : public JEventProcessor {

    Input<MyData> m_data_in {this};
    int last_event_nr = -1;

public:
    MyProc(bool enable_ordering=true) {
        SetPrefix("myproc");
        SetTypeName("MyProc");
        SetCallbackStyle(CallbackStyle::ExpertMode);
        EnableOrdering(enable_ordering);
    }

    void ProcessSequential(const JEvent& event) override {
        auto evt_nr = event.GetEventNumber();
        LOG_DEBUG(GetLogger()) << "Processing event nr " << evt_nr << " with index " << event.GetEventIndex();
        if (IsOrderingEnabled()) {
            REQUIRE(evt_nr == (size_t) last_event_nr+1);
            last_event_nr = evt_nr;
        }
    }
};

TEST_CASE("OrderingTests") {
    JApplication app;
    app.Add(new JEventSource);
    app.Add(new MyProc);
    app.Add(new JFactoryGeneratorT<MyFac>);
    app.SetParameterValue("jana:nevents", 20);
    app.SetParameterValue("myfac:loglevel", "debug");
    app.SetParameterValue("myproc:loglevel", "debug");
    app.SetParameterValue("nthreads", "4");
    app.Run();
}

TEST_CASE("OrderingTests_SequentialBaseline") {
    JApplication app;
    app.Add(new JEventSource);
    app.Add(new MyProc);
    app.Add(new JFactoryGeneratorT<MyFac>);
    app.SetParameterValue("jana:nevents", 20);
    app.SetParameterValue("myfac:loglevel", "debug");
    app.SetParameterValue("myproc:loglevel", "debug");
    app.SetParameterValue("nthreads", "1");
    app.Run();
}

TEST_CASE("OrderingTests_FullyParallel") {
    JApplication app;
    app.Add(new JEventSource);
    app.Add(new MyProc{false});
    app.Add(new JFactoryGeneratorT<MyFac>);
    app.SetParameterValue("jana:nevents", 20);
    app.SetParameterValue("myfac:loglevel", "debug");
    app.SetParameterValue("myproc:loglevel", "debug");
    app.SetParameterValue("nthreads", "4");
    app.Run();
}

} // namespace jana::engine::ordering_tests
