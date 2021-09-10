
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_COMPONENTTESTS_H
#define JANA2_COMPONENTTESTS_H


#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

struct SimpleSource : public JEventSource {

    std::atomic_int open_count {0};
    std::atomic_int event_count {0};

    SimpleSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
        open_count += 1;
    }

    void GetEvent(std::shared_ptr<JEvent>) override {
        if (++event_count == 5) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
    }
};


struct SimpleProcessor : public JEventProcessor {

    std::atomic_int init_count {0};
    std::atomic_int finish_count {0};

    SimpleProcessor(JApplication* app) : JEventProcessor(app) {}

    void Init() override {
        init_count += 1;
    }

    void Process(const std::shared_ptr<const JEvent>&) override {
    }

    void Finish() override {
        finish_count += 1;
    }
};


#endif //JANA2_COMPONENTTESTS_H
