
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JDISCRETEEVENTJOIN_H
#define JANA2_JDISCRETEEVENTJOIN_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>

#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Streaming/JTransport.h>
#include <JANA/Streaming/JTrigger.h>

/// JEventBuilder pulls JMessages off of a user-specified JTransport, aggregates them into
/// JEvents using the JWindow of their choice, and decides which to keep via a user-specified
/// JTrigger. The user can choose to merge this Event stream with additional JMessage streams, possibly
/// applying a different trigger at each level. This is useful for level 2/3/n triggers and maybe EPICS data.

template <typename T>
class JDiscreteJoin : public JEventSource {
public:

    JDiscreteJoin(std::unique_ptr<JTransport>&& transport,
                       std::unique_ptr<JTrigger>&& trigger = std::unique_ptr<JTrigger>(new JTrigger()))

            : JEventSource("JEventBuilder")
            , m_transport(std::move(transport))
            , m_trigger(std::move(trigger))
    {
    }

    void Open() override {
        m_transport->initialize();
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto item = new T();  // This is why T requires a zero-arg ctor
        auto result = m_transport->receive(*item);
        switch (result) {
            case JTransport::Result::FINISHED:
                throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JTransport::Result::TRY_AGAIN:
                throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JTransport::Result::FAILURE:
                throw JEventSource::RETURN_STATUS::kERROR;
            default:
                break;
        }
        // At this point, we know that item contains a valid Sample<T>

        event->SetEventNumber(m_next_id);
        m_next_id += 1;
        event->Insert<T>(item);
        std::cout << "Emit: " << *item << std::endl;
    }

    static std::string GetDescription() {
        return "JEventBuilder";
    }


private:

    std::unique_ptr<JTransport> m_transport;
    std::unique_ptr<JTrigger> m_trigger;
    uint64_t m_delay_ms;
    uint64_t m_next_id = 0;
};



#endif //JANA2_JEVENTJOINER_H
