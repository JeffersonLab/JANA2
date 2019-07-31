//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JSESSIONWINDOW_H
#define JANA2_JSESSIONWINDOW_H

#include <JANA/Streaming/JWindow.h>

#include <map>
#include <queue>

/// JSessionWindow aggregates JMessages adaptively, i.e. a JEvent's time interval starts with the
/// first JMessage and ends once there are no more JMessages timestamped before a configurable
/// max interval width. This is usually what is meant by 'event-building'.
template <typename T>
class JSessionWindow : public JWindow<T> {
private:
    template <typename S>
    struct maybe {
        bool is_none = true;
        S value;
    };

public:
    using Timestamp = JMessage::Timestamp;
    using Duration = JMessage::Timestamp;
    using DetectorId = JMessage::DetectorId;

    JSessionWindow(Timestamp event_interval, const std::vector<DetectorId> &detectors)
    : m_event_interval(event_interval) {
        for (auto id : detectors) {
            m_inbox.insert({id, {}});
        }
    }

    void pushMessage(T* message) final {
        // TODO: Why was messages a vector before?
        auto iter = m_inbox.find(message->source_id);
        if (iter == m_inbox.end()) {
            throw std::runtime_error("Unexpected detector!");
        } else {
            iter->second.push_back(message);
        }
    };

    bool pullEvent(JEvent& event) final {
        return false;
    };

/*
    maybe<JMessage::Timestamp> find_next_event_start() {
        maybe<JMessage::Timestamp> result;
        result.is_none = true;
        if (!m_inbox.empty()) {
            result.is_none = false;
            auto iter = m_inbox.begin();
            result.value = iter->second.timestamp;

            while (++iter != m_inbox.end()) {
                if (iter->second.is_none) {
                    result.is_none = true;
                    return result;
                }
                result.value = std::min(result.value, iter->second.value);
            }
        }
        return result;
    }

    bool stage_next_event() {
        if (m_next_event_start.is_none) {
            m_next_event_start = find_next_event_start();
            if (m_next_event_start.is_none) {
                return false;
            }
        }
        Timestamp next_event_finish = m_next_event_start.value + m_event_interval;
        for (auto iter : m_inbox) { // for each detector
            auto &q = iter.second;
            while (q.front().timestamp <= next_event_finish) {
                m_outbox.push_back(std::move(q.front()));
                q.pop_front();
            }
            if (q.empty()) {
                // we've run out of samples, so we can't say whether we have everything or not
                return false;
            }
        }
        return true;
    }

*/
private:
    std::map<JMessage::DetectorId, std::deque<JMessage*>> m_inbox;
    std::vector<T *> m_outbox;
    maybe<JMessage::Timestamp> m_next_event_start;
    JMessage::Timestamp m_event_interval; // TODO: This should be a duration
};


#endif //JANA2_JSESSIONWINDOW_H
