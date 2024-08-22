
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_SCALETESTS_H
#define JANA2_SCALETESTS_H
#include <JANA/Utils/JBenchUtils.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>

namespace scaletest {
struct DummySource : public JEventSource {

    DummySource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    Result Emit(JEvent& event) override {
        std::unique_ptr<JBenchUtils> bench_utils = std::make_unique<JBenchUtils>(event.GetEventNumber(), NAME_OF_THIS);
        bench_utils->consume_cpu_ms(20);
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        return Result::Success;
    }
};

struct DummyProcessor : public JEventProcessor {

    DummyProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Process(const JEvent& event) override {
        std::unique_ptr<JBenchUtils> bench_utils = std::make_unique<JBenchUtils>(event.GetEventNumber(), NAME_OF_THIS);
        bench_utils->consume_cpu_ms(100);
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
};
} // namespace scaletest
#endif //JANA2_SCALETESTS_H
