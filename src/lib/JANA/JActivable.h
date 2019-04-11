//
// Created by nbrei on 3/26/19.
//

#ifndef GREENFIELD_ACTIVABLE_H
#define GREENFIELD_ACTIVABLE_H

#include <vector>

/// Activable provides a way of propagating information about whether a particular
/// item in the topology is active or not using notifications. It is an instance of
/// the Observer pattern. It takes advantage of the property that the information always
/// flows downstream.

/// Queues and arrows together form a bipartite graph. (Queue vertices only have edges
/// to arrow vertices, and vice versa.) Activable acts like a common vertex type.
class JActivable {

private:
    bool _is_active = false;
    std::vector<JActivable *> _upstream;
    std::vector<JActivable *> _downstream;

public:
    virtual bool is_active() {
        return _is_active;
    }

    virtual void set_active(bool is_active) {
        _is_active = is_active;
    }

    void update_activeness(bool is_active) {
        if (is_active) {
            set_active(true);
            notify_downstream(true);
            // propagate this immediately
        }
        else {
            bool any_active = false;
            for (auto activable : _upstream) {
                any_active |= activable->is_active();
            }
            if (!any_active) {
                set_active(false);
                // defer propagating
            }
        }
    }

    void notify_downstream(bool is_active) {
        for (auto activable : _downstream) {
            activable->update_activeness(is_active);
        }
    }

    void attach_upstream(JActivable* activable) {
        _upstream.push_back(activable);
    };

    void attach_downstream(JActivable* activable) {
        _downstream.push_back(activable);
    };

};


#endif //GREENFIELD_ACTIVABLE_H
