//
// Created by Nathan Brei on 2019-07-28.
//

#ifndef JANA2_JEVENTBUILDER_H
#define JANA2_JEVENTBUILDER_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <queue>

#include <JANA/Streaming/JTransport.h>
#include <JANA/Streaming/JWindow.h>
#include <JANA/Streaming/JMergeWindow.h>

/// JTrigger determines whether an event contains data worth passing downstream, or whether
/// it should be immediately recycled. The user can call arbitrary JFactories from a Trigger
/// just like they can from an EventProcessor.
/// This design allows the user to reuse reconstruction code for a software trigger, and to
/// reuse results calculated for the trigger during reconstruction. The accept() function
/// should be thread safe, so that the trigger can be automatically parallelized, which will
/// help bound the system's overall latency.
struct JTrigger {

    virtual bool accept(JEvent &event) { return true; }

};


/// JEventBuilder pulls JMessages off of a user-specified JTransport, aggregates them into
/// JEvents using the JWindow of their choice, and decides which to keep via a user-specified
/// JTrigger. The user can choose to merge this Event stream with additional JMessage streams, possibly
/// applying a different trigger at each level. This is useful for level 2/3/n triggers and maybe EPICS data.

template <typename T>
class JEventBuilder : public JEventSource {
public:

    JEventBuilder(std::unique_ptr<JTransport<T>> transport,
                  std::unique_ptr<JWindow<T>> window,
                  std::unique_ptr<JTrigger> trigger = std::unique_ptr<JTrigger>())

        : JEventSource("JEventBuilder") {

        m_levels.emplace_back(std::move(transport), std::move(window), std::move(trigger));
    }

    void addLevel(std::unique_ptr<JTransport<T>> transport, std::unique_ptr<JTrigger> trigger = std::unique_ptr<JTrigger>()) {
        auto merge_window = std::unique_ptr<JMergeWindow<T>>();
        m_levels.push_back({transport, merge_window, trigger});
    }

    void Open() override {
        for (auto& level : m_levels) {
            level.transport->initialize();
        }
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {

        auto item = new T();  // This is why T requires a zero-arg ctor
        auto result = m_levels[0].transport->receive(*item);
        switch (result) {
            case JTransportResult::FINISHED:
                throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
            case JTransportResult::TRY_AGAIN:
                throw JEventSource::RETURN_STATUS::kTRY_AGAIN;
            case JTransportResult::FAILURE:
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
    struct Level {
        std::unique_ptr<JTransport<T>> transport;
        std::unique_ptr<JWindow<T>> window;
        std::unique_ptr<JTrigger> trigger;

        Level(std::unique_ptr<JTransport<T>>&& transport,
              std::unique_ptr<JWindow<T>>&& window,
              std::unique_ptr<JTrigger>&& trigger)
            : transport(std::move(transport)), window(std::move(window)), trigger(std::move(trigger)) {};
    };

    std::vector<Level> m_levels;

    uint64_t m_delay_ms;
    uint64_t m_next_id = 0;

};


#endif //JANA2_JEVENTBUILDER_H
