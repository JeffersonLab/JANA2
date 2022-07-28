
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef GREENFIELD_ACTIVABLE_H
#define GREENFIELD_ACTIVABLE_H

#include <vector>
#include <atomic>
#include <cassert>
#include <JANA/JException.h>

/// Activable provides a way of propagating information about whether a particular
/// item in the topology is active or not using notifications. It is an instance of
/// the Observer pattern. It takes advantage of the property that the information always
/// flows downstream.

class JActivable {
public:
    enum class Status {Unopened, Running, Stopped, Finished};

private:
    std::vector<JActivable *> m_listeners;
    std::atomic<int> m_running_upstreams {0};
    std::atomic<Status> m_status {Status::Unopened};

public:
    Status get_status() {
        return m_status;
    }

    int get_running_upstreams() {
        return m_running_upstreams;
    }

    void run() {
        assert(m_status == Status::Unopened || m_status == Status::Stopped || m_status == Status::Running);
        Status old_status = m_status;
        on_status_change(old_status, Status::Running);
        for (auto listener: m_listeners) {
            listener->m_running_upstreams++;
            listener->run();  // Activating something recursively activates everything downstream.
        }
        m_status = Status::Running;
    }

    void stop() {
        if (m_status != Status::Running) return; // stop() is a no-op unless running
        Status old_status = m_status;
        for (auto listener: m_listeners) {
            listener->m_running_upstreams--;
            // listener->stop();
            // This is not a sufficient condition for stopping downstream listeners.
            // What we need is zero running upstreams AND zero messages in queue AND zero threads currently processing
            // Correspondingly, the scheduler or worker needs to be the one to call stop() when this condition is reached.
        }
        on_status_change(old_status, Status::Stopped);
        m_status = Status::Stopped;
    }

    void finish() {
        Status old_status = m_status;

        if (old_status == Status::Running) {
            for (auto listener: m_listeners) {
                listener->m_running_upstreams--;
            }
        }
        if (old_status != Status::Finished) {
            on_status_change(old_status, Status::Finished);
        }
        m_status = Status::Finished;
    }

    virtual void on_status_change(Status /*old_status*/, Status /*new_status*/) {};

    void attach_listener(JActivable* listener) {
        m_listeners.push_back(listener);
    };

};


#endif //GREENFIELD_ACTIVABLE_H
