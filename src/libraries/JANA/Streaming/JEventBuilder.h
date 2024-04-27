
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTBUILDER_H
#define JANA2_JEVENTBUILDER_H

#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Streaming/JTransport.h>
#include <JANA/Streaming/JTrigger.h>
#include <JANA/Streaming/JDiscreteJoin.h>
#include <JANA/Streaming/JWindow.h>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>



/// JEventBuilder pulls JMessages off of a user-specified JTransport, aggregates them into
/// JEvents using the JWindow of their choice, and decides which to keep via a user-specified
/// JTrigger.

template <typename T>
class JEventBuilder : public JEventSource {
public:

    JEventBuilder(std::unique_ptr<JTransport>&& transport,
                  std::unique_ptr<JTrigger>&& trigger = std::unique_ptr<JTrigger>(new JTrigger()),
                  std::unique_ptr<JWindow<T>>&& window = std::unique_ptr<JSessionWindow<T>>(new JSessionWindow<T>()))

        : JEventSource("JEventBuilder")
        , m_transport(std::move(transport))
        , m_trigger(std::move(trigger))
        , m_window(std::move(window)) {
            SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void addJoin(std::unique_ptr<JDiscreteJoin<T>>&& join) {
        m_joins.push_back(std::move(join));
    }

    void Open() override {
        for (auto join : m_joins) {
            join->Open();
        }
    }

    static std::string GetDescription() {
        return "JEventBuilder";
    }

    Result Emit(JEvent&) override {

        auto item = new T();  // This is why T requires a zero-arg ctor
        auto result = m_transport->receive(*item);
        switch (result) {
            case JTransport::Result::FINISHED:
                return Result::FailureFinished;
            case JTransport::Result::TRY_AGAIN:
                return Result::FailureTryAgain;
            case JTransport::Result::FAILURE:
                throw JException("Transport failure!");
        }
        // At this point, we know that item contains a valid Sample<T>

        event.SetEventNumber(m_next_id);
        m_next_id += 1;
        event.Insert<T>(item);

        /// This is really bad because we have to worry about downstream HitSource returning TryAgainLater
        /// and we really don't want to block here
        for (auto join : m_joins) {
            join->GetEvent(event);
        }
        std::cout << "Emit: " << *item << std::endl;
        return Result::Success;
    }


private:
    std::unique_ptr<JTransport> m_transport;
    std::unique_ptr<JWindow<T>> m_window;
    std::unique_ptr<JTrigger> m_trigger;

    // Downstream joins should probably be managed externally,
    // since we will want these with regular EventSources as well
    std::vector<std::unique_ptr<JDiscreteJoin<T>>> m_joins;

    uint64_t m_delay_ms;
    uint64_t m_next_id = 0;

};


#endif //JANA2_JEVENTBUILDER_H
