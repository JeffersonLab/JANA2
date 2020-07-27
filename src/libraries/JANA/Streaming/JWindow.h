
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JWINDOW_H
#define JANA2_JWINDOW_H

#include <JANA/Streaming/JMessage.h>
#include <JANA/JEvent.h>

#include <queue>

/// JWindow is an abstract data structure for aggregating individual JMessages into a
/// single JEvent.  We generally assume that messages from any particular source arrive in-order, and
/// make no assumptions about ordering between different sources. We provide different implementations to fit
/// different use cases so that the user should rarely need to implement one of these by themselves.
/// The choice of JWindow determines how the time interval associated with each
/// JEvent is calculated, and furthermore is responsible for placing the correct JMessages into
/// the JEvent. As long as the time intervals do not overlap, this amounts to simple transferring
/// of ownership of a raw pointer. When JEvents intervals do overlap, which happens in the case of JSlidingWindow
/// and JMergeWindow, we have to decide whether to use shared ownership or to clone the offending data.
template <typename T>
struct JWindow {

    virtual ~JWindow() = default;
    virtual void pushMessage(T* message) = 0;
    virtual bool pullEvent(JEvent& event) = 0;

};



/// JFixedWindow partitions time into fixed, contiguous buckets, and emits a JEvent containing
/// all JMessages for all sources which fall into that bucket.
template <typename T>
class JFixedWindow : public JWindow<T> {
public:
    void pushMessage(T* message) final;
    bool pullEvent(JEvent& event) final;
private:
};


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
    std::map<DetectorId, std::deque<JMessage*>> m_inbox;
    std::vector<T *> m_outbox;
    maybe<Timestamp> m_next_event_start;
    Timestamp m_event_interval; // TODO: This should be a duration
};

#endif //JANA2_JWINDOW_H
