
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_BARRIEREVENTTESTS_H
#define JANA2_BARRIEREVENTTESTS_H

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

int global_resource = 0;

class BarrierSource : public JEventSource {
    int event_count=0;

public:
    BarrierSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "BarrierTests fake event source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        event_count++;

        if (event_count >= 100) {
            throw RETURN_STATUS::kNO_MORE_EVENTS;
        }

        LOG << "Emitting event " << event_count << LOG_END;
        event->SetEventNumber(event_count);

        if (event_count % 10 == 0) {
            event->SetSequential(true);
        }

    }
};



struct BarrierProcessor : public JEventProcessor {

public:
    explicit BarrierProcessor(JApplication* app) : JEventProcessor(app) {}

    void Process(const std::shared_ptr<const JEvent>& event) override {

        if (event->GetSequential()) {
            global_resource += 1;
            LOG << "Barrier event = " << event->GetEventNumber() << ", writing global var = " << global_resource << LOG_END;
        }
        else {
            LOG << "Processing non-barrier event = " << event->GetEventNumber() << ", reading global var = " << global_resource << LOG_END;
        }
    }
};

#endif //JANA2_BARRIEREVENTTESTS_H
