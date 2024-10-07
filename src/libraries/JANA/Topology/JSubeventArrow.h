
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "JMailbox.h"
#include "JArrow.h"
#include <JANA/JEvent.h>

/// SubeventProcessor offers sub-event-level parallelism. The idea is to split parent
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

template <typename InputT, typename OutputT>
struct JSubeventProcessor {

    std::string inputTag;
    std::string outputTag;

    virtual OutputT* ProcessSubevent(InputT* input) = 0;

};

template <typename SubeventT>
struct SubeventWrapper {

    std::shared_ptr<JEvent>* parent;
    SubeventT* data;
    size_t id;
    size_t total;

    SubeventWrapper(std::shared_ptr<JEvent>* parent, SubeventT* data, size_t id, size_t total)
    : parent(std::move(parent))
    , data(data)
    , id(id)
    , total(total) {}
};


template <typename InputT, typename OutputT>
class JSubeventArrow : public JArrow {
    JSubeventProcessor<InputT, OutputT>* m_processor;
    JMailbox<SubeventWrapper<InputT>>* m_inbox;
    JMailbox<SubeventWrapper<OutputT>>* m_outbox;
public:
    JSubeventArrow(std::string name,
                   JSubeventProcessor<InputT,OutputT>* processor,
                   JMailbox<SubeventWrapper<InputT>>* inbox,
                   JMailbox<SubeventWrapper<OutputT>>* outbox)
        : JArrow(name, true, false, false), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {
    }

    size_t get_pending() final { return m_inbox->size(); };
    size_t get_threshold() final { return m_inbox->get_threshold(); };
    void set_threshold(size_t threshold) final { m_inbox->set_threshold(threshold);};

    void execute(JArrowMetrics&, size_t location_id) override;
};

template <typename InputT, typename OutputT>
class JSplitArrow : public JArrow {
    JSubeventProcessor<InputT, OutputT>* m_processor;
    JMailbox<std::shared_ptr<JEvent>*>* m_inbox;
    JMailbox<SubeventWrapper<InputT>>* m_outbox;
public:
    JSplitArrow(std::string name,
                JSubeventProcessor<InputT,OutputT>* processor,
                JMailbox<std::shared_ptr<JEvent>*>* inbox,
                JMailbox<SubeventWrapper<InputT>>* outbox)
        : JArrow(name, true, false, false), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {
    }

    size_t get_pending() final { return m_inbox->size(); };
    size_t get_threshold() final { return m_inbox->get_threshold(); };
    void set_threshold(size_t threshold) final { m_inbox->set_threshold(threshold);};

    void execute(JArrowMetrics&, size_t location_id) override;
};

template <typename InputT, typename OutputT>
class JMergeArrow : public JArrow {
    JSubeventProcessor<InputT,OutputT>* m_processor;
    JMailbox<SubeventWrapper<OutputT>>* m_inbox;
    JMailbox<std::shared_ptr<JEvent>*>* m_outbox;
    std::map<std::shared_ptr<JEvent>*, size_t> m_in_progress;
public:
    JMergeArrow(std::string name,
                JSubeventProcessor<InputT,OutputT>* processor,
                JMailbox<SubeventWrapper<OutputT>>* inbox,
                JMailbox<std::shared_ptr<JEvent>*>* outbox)
        : JArrow(name, false, false, false), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {
    }

    size_t get_pending() final { return m_inbox->size(); };
    size_t get_threshold() final { return m_inbox->get_threshold(); };
    void set_threshold(size_t threshold) final { m_inbox->set_threshold(threshold);};
    void execute(JArrowMetrics&, size_t location_id) override;
};



template <typename InputT, typename OutputT>
void JSplitArrow<InputT, OutputT>::execute(JArrowMetrics& result, size_t location_id) {
    using InQueue = JMailbox<std::shared_ptr<JEvent>*>;
    using OutQueue = JMailbox<SubeventWrapper<InputT>>;
    auto start_total_time = std::chrono::steady_clock::now();

    std::shared_ptr<JEvent>* event = nullptr;
    bool success;
    size_t reserved_size = m_outbox->reserve(1);
    size_t actual_size = reserved_size;
    // TODO: Exit early if we don't have enough space on output queue

    auto in_status = m_inbox->pop(event, success, location_id);
    auto start_latency_time = std::chrono::steady_clock::now();

    std::vector<SubeventWrapper<InputT>> wrapped;
    if (success) {

        // Construct prereqs
        std::vector<const InputT*> originals = (*event)->Get<InputT>(m_processor->inputTag);
        size_t i = 1;
        actual_size = originals.size();

        for (const InputT* original : originals) {
            InputT* unconsted = const_cast<InputT*>(original); // TODO: Get constness right
            wrapped.push_back(SubeventWrapper<InputT>(event, unconsted, i++, actual_size));
        }
    }
    auto end_latency_time = std::chrono::steady_clock::now();
    auto out_status = OutQueue::Status::Ready;

    size_t output_size = wrapped.size();
    if (success) {
        assert(m_outbox != nullptr);
        out_status = m_outbox->push(wrapped, reserved_size, location_id);
    }
    auto end_queue_time = std::chrono::steady_clock::now();

    JArrowMetrics::Status status;
    if (in_status == InQueue::Status::Ready && out_status == OutQueue::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    auto latency = (end_latency_time - start_latency_time);
    auto overhead = (end_queue_time - start_total_time) - latency;
    result.update(status, output_size, 1, latency, overhead);

}
template <typename InputT, typename OutputT>
void JSubeventArrow<InputT, OutputT>::execute(JArrowMetrics& result, size_t location_id) {
    using SubeventQueue = JMailbox<SubeventWrapper<InputT>>;
    auto start_total_time = std::chrono::steady_clock::now();

    // TODO: Think more carefully about subevent bucket size
    std::vector<SubeventWrapper<InputT>> inputs;
    size_t downstream_accepts = m_outbox->reserve(1, location_id);
    auto in_status = m_inbox->pop(inputs, downstream_accepts, location_id);
    auto start_latency_time = std::chrono::steady_clock::now();

    std::vector<SubeventWrapper<OutputT>> outputs;
    for (const auto& input : inputs) {
        auto output = m_processor->ProcessSubevent(input.data);
        auto wrapped = SubeventWrapper<OutputT>(input.parent, output, input.id, input.total);
        outputs.push_back(wrapped);
    }
    size_t outputs_size = outputs.size();
    auto end_latency_time = std::chrono::steady_clock::now();
    auto out_status = JMailbox<SubeventWrapper<OutputT>>::Status::Ready;

    if (outputs_size > 0) {
        assert(m_outbox != nullptr);
        out_status = m_outbox->push(outputs, downstream_accepts, location_id);
    }
    auto end_queue_time = std::chrono::steady_clock::now();

    JArrowMetrics::Status status;
    if (in_status == SubeventQueue::Status::Ready && out_status == JMailbox<SubeventWrapper<OutputT>>::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    auto latency = (end_latency_time - start_latency_time);
    auto overhead = (end_queue_time - start_total_time) - latency;
    result.update(status, outputs_size, 1, latency, overhead);

}
template <typename InputT, typename OutputT>
void JMergeArrow<InputT, OutputT>::execute(JArrowMetrics& result, size_t location_id) {
    using InQueue = JMailbox<SubeventWrapper<OutputT>>;
    using OutQueue = JMailbox<std::shared_ptr<JEvent>*>;

    auto start_total_time = std::chrono::steady_clock::now();

    // TODO: Think more carefully about subevent bucket size
    std::vector<SubeventWrapper<OutputT>> inputs;
    size_t downstream_accepts = m_outbox->reserve(1, location_id);
    auto in_status = m_inbox->pop(inputs, downstream_accepts, location_id);
    auto start_latency_time = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<JEvent>*> outputs;
    for (const auto& input : inputs) {
        LOG_TRACE(m_logger) << "JMergeArrow: Processing input with parent=" << input.parent << ", evt=" << (*(input.parent))->GetEventNumber() << ", sub=" << input.id << " and total=" << input.total << LOG_END;
        // Problem: Are we sure we are updating the event in a way which is effectively thread-safe?
        // Should we be doing this insert, or should the caller?
        (*(input.parent))->template Insert<OutputT>(input.data);
        if (input.total == 1) {
            // Goes straight into "ready"
            outputs.push_back(input.parent);
            LOG_TRACE(m_logger) << "JMergeArrow: Finished parent=" << input.parent << ", evt=" << (*(input.parent))->GetEventNumber() << LOG_END;
        }
        else {
            auto pair = m_in_progress.find(input.parent);
            if (pair == m_in_progress.end()) {
                m_in_progress[input.parent] = input.total-1;
            }
            else {
                if (pair->second == 0) {   // Event was already in the map. TODO: What happens when we turn off event recycling?
                    pair->second = input.total-1;
                }
                else if (pair->second == 1) {
                    pair->second -= 1;
                    outputs.push_back(input.parent);
                    LOG_TRACE(m_logger) << "JMergeArrow: Finished parent=" << input.parent << ", evt=" << (*(input.parent))->GetEventNumber() << LOG_END;
                }
                else {
                    pair->second -= 1;
                }
            }
        }
    }
    LOG_DEBUG(m_logger) << "MergeArrow consumed " << inputs.size() << " subevents, produced " << outputs.size() << " events" << LOG_END;
    auto end_latency_time = std::chrono::steady_clock::now();

    auto outputs_size = outputs.size();
    auto out_status = m_outbox->push(outputs, downstream_accepts, location_id);

    auto end_queue_time = std::chrono::steady_clock::now();

    JArrowMetrics::Status status;
    if (in_status == InQueue::Status::Ready && out_status == OutQueue::Status::Ready && inputs.size() > 0) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    auto latency = (end_latency_time - start_latency_time);
    auto overhead = (end_queue_time - start_total_time) - latency;
    result.update(status, outputs_size, 1, latency, overhead);
}



