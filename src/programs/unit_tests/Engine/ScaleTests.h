
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
        m_bench_utils.set_seed(event.GetEventNumber(), NAME_OF_THIS);
        m_bench_utils.consume_cpu_ms(20);
        return Result::Success;
    }

    private:
        JBenchUtils m_bench_utils = JBenchUtils();
};

struct DummyData {int x;};
struct DummyFactory : public JFactoryT<DummyData> {
    JBenchUtils m_bench_utils = JBenchUtils();

    void Process(const std::shared_ptr<const JEvent>& event) override {
        m_bench_utils.set_seed(event->GetEventNumber(), NAME_OF_THIS);
        m_bench_utils.consume_cpu_ms(70);
        Insert(new DummyData {22});
    }
};

struct DummyProcessor : public JEventProcessor {

    DummyProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void ProcessParallel(const JEvent& event) override {
        event.Get<DummyData>();
    }

    void Process(const JEvent& event) override {
        m_bench_utils.set_seed(event.GetEventNumber(), NAME_OF_THIS);
        m_bench_utils.consume_cpu_ms(10);
    }

    private:
        JBenchUtils m_bench_utils = JBenchUtils();
};
} // namespace scaletest
#endif //JANA2_SCALETESTS_H
