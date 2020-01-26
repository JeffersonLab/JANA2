//
// Created by Nathan Brei on 2019-07-19.
//

#ifndef JANA2_TERMINATIONTESTS_H
#define JANA2_TERMINATIONTESTS_H

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>



struct BoundedSource : public JEventSource {

    std::atomic_int event_count {0};

    BoundedSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        if (event_count >= 10) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        event_count += 1;
    }
};

struct UnboundedSource : public JEventSource {

    std::atomic_int event_count {0};

    UnboundedSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        event_count += 1;
    }
};

struct CountingProcessor : public JEventProcessor {

    std::atomic_int processed_count {0};

    CountingProcessor(JApplication* app) : JEventProcessor(app) {}

    void Init() override {}

    void Process(const std::shared_ptr<const JEvent>& aEvent) override {
        processed_count += 1;
    }

    void Finish() override {
    }
};


#endif //JANA2_TERMINATIONTESTS_H
