
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_SCALETESTS_H
#define JANA2_SCALETESTS_H
#include <JANA/Utils/JPerfUtils.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>

namespace scaletest {
struct DummySource : public JEventSource {

    void GetEvent(std::shared_ptr<JEvent>) override {
        consume_cpu_ms(20);
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
};

struct DummyProcessor : public JEventProcessor {

    void Process(const std::shared_ptr<const JEvent> &) override {
        consume_cpu_ms(100);
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
};
} // namespace scaletest
#endif //JANA2_SCALETESTS_H
