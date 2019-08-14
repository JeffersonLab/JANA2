//
// Created by Nathan Brei on 2019-07-28.
//

#ifndef JANA2_JSTREAMINGEVENTSOURCE_H
#define JANA2_JSTREAMINGEVENTSOURCE_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>

#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Streaming/JTransport.h>

/// JStreamingEventSource makes it convenient to stream events into JANA by handling transport and
/// message type as separate, orthogonal concerns.
template <typename T>
class JStreamingEventSource : public JEventSource {

    std::unique_ptr<JTransport> m_transport;
    T* m_next_item;
    size_t m_next_evt_nr = 1;

public:

    explicit JStreamingEventSource(std::unique_ptr<JTransport>&& transport)
        : JEventSource("JStreamingEventSource")
        , m_transport(std::move(transport))
        , m_next_item(nullptr)
    {
    }

    void Open() override {
        m_transport->initialize();
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        if (m_next_item == nullptr) {
            m_next_item = new T();  // This is why T requires a zero-arg ctor
        }

        auto result = m_transport->receive(*m_next_item);
        switch (result) {
            case JTransport::Result::FINISHED:   throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JTransport::Result::TRY_AGAIN:  throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JTransport::Result::FAILURE:    throw JEventSource::RETURN_STATUS::kERROR;
            default:                             break;
        }

        // At this point, we know that item contains a valid JEventMessage
        T* item = m_next_item;
        m_next_item = nullptr;

        size_t evt_nr = item->get_event_number();
        event->SetEventNumber(evt_nr == 0 ? m_next_evt_nr++ : evt_nr);
        event->SetRunNumber(item->get_run_number());
        event->Insert<T>(item);
        std::cout << "Recv: " << *item << std::endl;
    }

    static std::string GetDescription() {
        return "JStreamingEventSource";
    }
};


#endif //JANA2_JSTREAMINGEVENTSOURCE_H
