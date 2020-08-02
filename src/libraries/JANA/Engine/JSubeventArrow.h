
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JSUBEVENTARROW_H
#define JANA2_JSUBEVENTARROW_H


#include "JMailbox.h"
#include "JArrow.h"
#include <JANA/JEvent.h>

using Event = std::shared_ptr<JEvent>;


/// Data structure containing all of the metadata needed to merge subevents
/// on a per-event basis without needing a barrier. This is used internally by
/// Queues and Arrows and the user should never need to interact with it directly
struct JSubevent {
    JEvent* parent;
    JObject* subevent_data;
    long subevent_id;
    long subevent_count;
};



/// SubtaskProcessor offers sub-event-level parallelism. The idea is to split parent
/// event S into independent subtasks T, and automatically bundling them with
/// bookkeeping information X onto a Queue<pair<T,X>. process :: T -> U handles the stateless,
/// parallel parts; its Arrow pushes messages on to a Queue<pair<U,X>, so that merge() :: S -> [U] -> V
/// "joins" all completed "subtasks" of type U corresponding to one parent of type S, (i.e.
/// a specific JEvent), back into a single entity of type V, (most likely the same JEvent as S,
/// only now containing more data) which is pushed onto a Queue<V>, bookkeeping information now gone.
/// Note that there is no blocking and that our streaming paradigm is not compromised.

/// Abstract class which is meant to extended by the user to contain all
/// subtask-related functions. (Data lives in a JObject instead)
/// Future versions might be templated for two reasons:
///  1. To make the functions non-virtual,
///  2. To replace the generic JObject pointer with something typesafe
/// Future versions could also recycle JObjects by using another Queue.
class JSubeventProcessor {

    virtual void split(JEvent& parent, std::vector<JObject*>& subevents) = 0;

    virtual void process(JObject& subevent) = 0;

    virtual void merge(JEvent& parent, std::vector<JObject*>& subevents) = 0;

};

class JSubeventArrow : public JArrow {
    JSubeventProcessor* _processor;
    JMailbox<JSubevent>* _inbox;
    JMailbox<JSubevent>* _outbox;
public:
    JSubeventArrow(std::string name, JSubeventProcessor* processor,
                   JMailbox<JSubevent>* inbox, JMailbox<JSubevent>* outbox);
    void execute(JArrowMetrics&, size_t location_id) override;
    size_t get_pending() final;
    size_t get_threshold() final;
    void set_threshold(size_t) final;
};

class JSplitArrow : public JArrow {
    JSubeventProcessor* _processor;
    JMailbox<Event>* _inbox;
    JMailbox<JSubevent>* _outbox;
public:
    JSplitArrow(std::string name, JSubeventProcessor* processor,
                JMailbox<Event>* inbox, JMailbox<JSubevent>* outbox);
    void execute(JArrowMetrics&, size_t location_id) override;
    size_t get_pending() final;
    size_t get_threshold() final;
    void set_threshold(size_t) final;
};

class JMergeArrow : public JArrow {
    JSubeventProcessor* _processor;
    JMailbox<JSubevent>* _inbox;
    JMailbox<Event>* _outbox;
public:
    JMergeArrow(std::string name, JSubeventProcessor* processor,
                JMailbox<JSubevent>* inbox, JMailbox<Event>* outbox);
    void execute(JArrowMetrics&, size_t location_id) override;
    size_t get_pending() final;
    size_t get_threshold() final;
    void set_threshold(size_t) final;
};


#endif //JANA2_JSUBEVENTARROW_H


