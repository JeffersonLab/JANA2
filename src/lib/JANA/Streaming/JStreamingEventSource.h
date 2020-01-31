//
// Created by Nathan Brei on 2019-07-28.
//

#ifndef JANA2_JSTREAMINGEVENTSOURCE_H
#define JANA2_JSTREAMINGEVENTSOURCE_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>

#include <JANA/JEventSource.h>
#include <JANA/Streaming/JTransport.h>

/// JStreamingEventSource is a class template which simplifies streaming events into JANA.
///
/// JStreamingEventSource makes it convenient to stream existing events into JANA by handling transport and message
/// format as separate, orthogonal concerns. The user need only implement classes satisfying the JTransport and
/// JEventMessage interfaces, and then they get a JStreamingEventSource 'for free'.
///
/// JStreamingEventSource<T> is templated on some message type T, which is a subclass of JEventMessage.
/// This is needed so that it can construct new message objects of the appropriate subclass.
/// As a result, JStreamingEventSource<T> only emits JEvents containing JEventMessages of type T,
/// which means that every message produced upstream must be compatible with that message format.
/// This shouldn't be a problem: it is always possible to make the message format more flexible, and any associated
/// complexity is fundamentally a property of the message format anyway. However, if we are using JStreamingEventSource,
/// it is essential that each message corresponds to one JEvent.
///
/// The JStreamingEventSource owns its JTransport, but passes ownership of each JMessage to its enclosing JEvent.

template <class MessageT>
class JStreamingEventSource : public JEventSource {

    std::unique_ptr<JTransport> m_transport;   ///< Pointer to underlying transport
    MessageT* m_next_item;     ///< An empty message buffer kept in reserve for when the next receive() succeeds
    size_t m_next_evt_nr = 1;  ///< If the event number is not encoded in the message payload, be able to assign one

public:

    /// The constructor requires a unique pointer to a JTransport implementation. This is a reasonable assumption to
    /// make because each JEventSource already corresponds to some unique resource. JStreamingEventSource should be free
    /// to destroy its transport object whenever it likes, so try to keep the JTransport free of weird shared state.

    explicit JStreamingEventSource(std::unique_ptr<JTransport>&& transport)
        : JEventSource("JStreamingEventSource")
        , m_transport(std::move(transport))
        , m_next_item(nullptr)
    {
    }

    /// Open delegates down to the transport, which will open a network socket or similar.

    void Open() override {
        m_transport->initialize();
    }

    /// GetEvent attempts to receive a JEventMessage. If it succeeds, it inserts it into the JEvent and sets
    /// the event number and run number appropriately.

    void GetEvent(std::shared_ptr<JEvent> event) override {

        if (m_next_item == nullptr) {
            m_next_item = new MessageT(GetApplication());
        }

        auto result = m_transport->receive(*m_next_item);
        switch (result) {
            case JTransport::Result::FINISHED:   throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JTransport::Result::TRY_AGAIN:  throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JTransport::Result::FAILURE:    throw JEventSource::RETURN_STATUS::kERROR;
            default:                             break;
        }

        // At this point, we know that item contains a valid JEventMessage
        MessageT* item = m_next_item;
        m_next_item = nullptr;

        size_t evt_nr = item->get_event_number();
        event->SetEventNumber(evt_nr == 0 ? m_next_evt_nr++ : evt_nr);
        event->SetRunNumber(item->get_run_number());
        event->Insert<MessageT>(item);
        std::cout << *item;
    }

    static std::string GetDescription() {
        return "JStreamingEventSource";
    }
};


#endif //JANA2_JSTREAMINGEVENTSOURCE_H
