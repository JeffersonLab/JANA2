
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef GREENFIELD_ACTIVABLE_H
#define GREENFIELD_ACTIVABLE_H

#include <vector>
#include <atomic>

/// Activable provides a way of propagating information about whether a particular
/// item in the topology is active or not using notifications. It is an instance of
/// the Observer pattern. It takes advantage of the property that the information always
/// flows downstream.

/// Queues and arrows together form a bipartite graph. (Queue vertices only have edges
/// to arrow vertices, and vice versa.) Activable acts like a common vertex type.
class JActivable {
public:
    enum class Status {Unopened, Inactive, Running, Draining, Drained, Finished, Closed};

private:
    std::vector<JActivable *> m_upstream;
    std::vector<JActivable *> m_downstream;

protected:
    std::atomic<Status> m_status {Status::Unopened};

public:
    virtual bool is_active() {
        return m_status == Status::Running || m_status == Status::Draining || m_status == Status::Drained;
    }

    virtual void set_active(bool is_active) {
        if (is_active) {
            m_status = Status::Running;
        }
        else {
            //assert(m_status == Status::Running || m_status == Status::Draining || m_status == Status::Drained);
            m_status = Status::Inactive;
        }
    }

    void update_activeness(bool is_active) {
        if (is_active) {
            set_active(true);
            notify_downstream(true);
            // propagate this immediately
        }
        else {
            bool any_active = false;
            for (auto activable : m_upstream) {
                any_active |= activable->is_active();
            }
            if (!any_active) {
                set_active(false);
                // defer propagating
            }
        }
    }

    void notify_downstream(bool is_active) {
        for (auto activable : m_downstream) {
            activable->update_activeness(is_active);
        }
    }

    void attach_upstream(JActivable* activable) {
        m_upstream.push_back(activable);
    };

    void attach_downstream(JActivable* activable) {
        m_downstream.push_back(activable);
    };

};


#endif //GREENFIELD_ACTIVABLE_H
