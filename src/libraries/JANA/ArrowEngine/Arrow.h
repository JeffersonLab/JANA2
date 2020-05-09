
#ifndef JANA2_ARROW_H
#define JANA2_ARROW_H

#include <string>
#include <functional>
#include <vector>
#include <JANA/ArrowEngine/Mailbox.h>
#include <JANA/Engine/JArrowMetrics.h>

namespace jana {
namespace arrowengine {

struct Arrow {
    std::string name;
    bool is_parallel;
    int chunk_size = 10;
    bool is_finished = false;
    std::atomic_int active_upstream_count {0};
    std::atomic_int thread_count {0};
    std::vector<Arrow*> downstreams;

    virtual void execute(JArrowMetrics& result, size_t location_id) = 0;

    Arrow(std::string name, bool is_parallel) : name(std::move(name)), is_parallel(is_parallel) {};
    virtual ~Arrow() = default;
};


template <typename T>
struct ArrowWithBasicOutbox : public virtual Arrow {
    Mailbox<T>* m_outbox; // Outbox is owned by the downstream arrows
};

template <typename T>
struct ArrowWithBasicInbox : public virtual Arrow {
    Mailbox<T> m_inbox; // Arrow owns its inbox(es), but not its outbox(es)
};


template <typename T>
class SourceOp {
public:
    enum class Status { Success, FailTryAgain, FailFinished };
    virtual std::pair<Status, T> next() = 0;
};

template <typename T, class MySourceOp>
struct SourceArrow : public ArrowWithBasicOutbox<T>, public MySourceOp {

    template<typename ...Args>
    SourceArrow(std::string name, bool is_parallel, Args... args) : Arrow(name, is_parallel), MySourceOp(args...) {
        static_assert(std::is_base_of<SourceOp<T>, MySourceOp>::value, "MySourceOp needs to be a subclass of SourceOp");
    };
    //SourceArrow(MySourceOp* )
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T, typename MySourceOp>
SourceArrow<T, MySourceOp> lift(SourceOp<T>* underlying) {
    // auto item = static_cast<SourceOp<T>>(underlying);
    return SourceArrow<T, MySourceOp>(underlying);
}

template <typename T>
class SinkOp {
public:
    virtual void accumulate(T item) = 0;
};

template <typename T, typename MySinkOp>
struct SinkArrow : public ArrowWithBasicInbox<T>, public MySinkOp {
    SinkArrow(std::string name, bool is_parallel)
    : Arrow(name, is_parallel) {
        static_assert(std::is_base_of<SinkOp<T>, MySinkOp>::value, "MySinkOp needs to be a subclass of SinkOp");
    }
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T, typename U>
class MapOp {
public:
    virtual U map(T) = 0;
};

template <typename T, typename U, typename MyMapOp>
struct StageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U>, public MyMapOp {
    template <typename ...Args>
    StageArrow(std::string name, bool is_parallel, Args... args) : Arrow(name, is_parallel), MyMapOp(args...) {
        static_assert(std::is_base_of<MapOp<T,U>, MyMapOp>::value, "MyMapOp needs to be a subclass of MapOp");
    };
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T>
struct BroadcastArrow : public ArrowWithBasicInbox<T> {
    std::vector<Mailbox<T>*> m_outboxes;
    BroadcastArrow(std::string name, bool is_parallel) : Arrow(name, is_parallel) {}
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T, typename U>
class MultiMapOp {
public:
    enum class Status { KeepGoing, NeedIngest };
    virtual void ingest(T);
    virtual Status emit(size_t requested_count, std::vector<U>& destination);
};

template <typename T, typename U, typename MyMultiMapOp>
struct MultiStageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U>, public MyMultiMapOp {
    MultiStageArrow(std::string name, bool is_parallel) : Arrow(name, is_parallel) {
        static_assert(std::is_base_of<MultiMapOp<T,U>, MyMultiMapOp>::value, "MyMultiMapOp needs to be a subclass of MultiMapOp");
    }
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T, typename U>
class ScatterOp {
public:
    virtual std::pair<U,size_t> scatter(T) = 0;
};

template <typename T, typename U, typename MyScatterOp>
struct ScatterArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U>, MyScatterOp {
    ScatterArrow(std::string name, bool is_parallel) : Arrow(name, is_parallel) {
        static_assert(std::is_base_of<ScatterOp<T, U>, MyScatterOp>::value, "MyScatterOp needs to be a subclass of ScatterOp");
    }
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T, typename U, typename V>
class SplitOp {
public:
    // In an ideal world we could just use std::variant
    struct Either {
        enum class Tag { Left, Right };
        union Val { U left; V right; };
        Tag tag;
        Val val;
    };
    virtual Either split(T input) = 0;
};


template <typename T, typename U, typename V>
struct SplitArrow : public ArrowWithBasicInbox<T> {
    Mailbox<U>* m_outbox_1;
    Mailbox<V>* m_outbox_2;
public:
    SplitArrow(std::string name, bool is_parallel) : Arrow(name, is_parallel) {};
    void execute(JArrowMetrics& result, size_t location_id) override;
};


template <typename T, typename U, typename V>
struct MergeOp {
    virtual V visit(T t) = 0;
    virtual V visit(U u) = 0;
};

template <typename T, typename U, typename V>
struct MergeArrow : public ArrowWithBasicOutbox<V> {
    Mailbox<T> m_inbox_1;
    Mailbox<U> m_inbox_2;
    MergeArrow(std::string name, bool is_parallel) : Arrow(name, is_parallel) {};
    void execute(JArrowMetrics& result, size_t location_id) override;
};

template <typename T>
void attach(ArrowWithBasicOutbox<T>& upstream, ArrowWithBasicInbox<T>& downstream) {
    upstream.m_outbox = &downstream.m_inbox;
    downstream.active_upstream_count += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename U, typename V, typename Derived>
void attach(BroadcastArrow<T>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.m_outboxes.push_back(&downstream.m_inbox);
    downstream.active_upstreams += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename U, typename V>
void attach(SplitArrow<T,U,V>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.m_outbox_1 = &downstream.m_inbox;
    downstream.active_upstreams += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T>& upstream, MergeArrow<T,U,V>& downstream) {
    upstream.m_outbox = &downstream.m_inbox_1;
    downstream.active_upstream_count += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T> upstream, MergeArrow<T,U,V> downstream) {
    upstream.m_outbox = &downstream.m_inbox_2;
    downstream.active_upstream_count += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename MySourceOp>
void SourceArrow<T, MySourceOp>::execute(JArrowMetrics& result, size_t location_id) {

    if (Arrow::is_finished) {
        result.update_finished();
        return;
    }
    JArrowMetrics::Status status;
    JArrowMetrics::duration_t latency;
    JArrowMetrics::duration_t overhead; // TODO: Populate these
    size_t message_count = 0;

    int requested_count = Arrow::chunk_size;
    std::vector<T> chunk_buffer; // TODO: Get rid of allocations
    int reserved_count = this->m_outbox->reserve(requested_count, location_id);

    if (reserved_count != 0) {

        using Status = typename MySourceOp::Status;
        Status lambda_result = Status::Success;
        for (int i=0; i<reserved_count && lambda_result == Status::Success; ++i) {
            auto pair = MySourceOp::next();
            lambda_result = pair.first;
            if (lambda_result == Status::Success) {
                chunk_buffer.push_back(pair.second);
            }
        }

        // We have to return our reservation regardless of whether our pop succeeded
        this->m_outbox->push(chunk_buffer, reserved_count, location_id);

        if (lambda_result == Status::Success) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else if (lambda_result == Status::FailTryAgain) {
            status = JArrowMetrics::Status::ComeBackLater;
            // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
        }
        else if (lambda_result == Status::FailFinished) {
            status = JArrowMetrics::Status::Finished;
        }
    }
    else { // reserved_count = 0  => downstream is full
        status = JArrowMetrics::Status::ComeBackLater;
        // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
    }
    result.update(status, message_count, 1, latency, overhead);
}

template <typename T, typename MySinkOp>
void SinkArrow<T, MySinkOp>::execute(JArrowMetrics& result, size_t location_id) {

    if (Arrow::is_finished) {
        result.update_finished();
        return;
    }

    JArrowMetrics::Status status;
    JArrowMetrics::duration_t latency;
    JArrowMetrics::duration_t overhead; // TODO: Populate these
    size_t message_count = 0;
    std::vector<T> chunk_buffer; // TODO: Get rid of allocations

    auto pop_result = this->m_inbox.pop(chunk_buffer, Arrow::chunk_size, location_id);

    // TODO: Timing information
    for (auto t : chunk_buffer) {
        MySinkOp::accumulate(t);
    }

    if (pop_result == Mailbox<T>::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
        status = JArrowMetrics::Status::Finished;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
        // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
    }
    result.update(status, message_count, 1, latency, overhead);
}

template <typename T, typename U, typename MyMapOp>
void StageArrow<T,U, MyMapOp>::execute(JArrowMetrics& result, size_t location_id) {

    if (Arrow::is_finished) {
        result.update_finished();
        return;
    }
    JArrowMetrics::Status status;
    JArrowMetrics::duration_t latency;
    JArrowMetrics::duration_t overhead; // TODO: Populate these
    size_t message_count = 0;

    int requested_count = Arrow::chunk_size;
    std::vector<T> input_chunk_buffer; // TODO: Get rid of allocations
    std::vector<U> output_chunk_buffer; // TODO: Get rid of allocations

    int reserved_count = this->m_outbox->reserve(requested_count, location_id);
    if (reserved_count != 0) {
        auto pop_result = this->m_inbox.pop(input_chunk_buffer, this->chunk_size, location_id);

        // TODO: Timing information
        for (auto t : input_chunk_buffer) {
            output_chunk_buffer.push_back(MyMapOp::map(t));
        }

        // We have to return our reservation regardless of whether our pop succeeded
        this->m_outbox->push(output_chunk_buffer, reserved_count, location_id);

        if (pop_result == Mailbox<T>::Status::Ready) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
            status = JArrowMetrics::Status::Finished;
        }
        else {
            status = JArrowMetrics::Status::ComeBackLater;
            // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
        }
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
        // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
    }
    result.update(status, message_count, 1, latency, overhead);
}


template <typename T>
void BroadcastArrow<T>::execute(JArrowMetrics& result, size_t location_id) {

    if (Arrow::is_finished) {
        result.update_finished();
        return;
    }
    JArrowMetrics::Status status;
    JArrowMetrics::duration_t latency;
    JArrowMetrics::duration_t overhead; // TODO: Populate these
    size_t message_count = 0;

    int requested_count = this->chunk_size;
    int dest_count = m_outboxes.size();
    std::vector<int> reservations(dest_count);
    for (int i=0; i<dest_count; ++i) {
        int reserved_count = m_outboxes[i]->reserve(requested_count, location_id);
        if (reserved_count < requested_count) requested_count = reserved_count;
        reservations[i] = reserved_count;
    }
    std::vector<T> chunk_buffer; // TODO: Get rid of allocations
    if (requested_count != 0) {
        auto pop_result = this->m_inbox.pop(chunk_buffer, requested_count, location_id);
        if (pop_result == Mailbox<T>::Status::Ready) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
            status = JArrowMetrics::Status::Finished;
        }
        else {
            status = JArrowMetrics::Status::ComeBackLater;
            // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
        }
        for (int i=0; i<dest_count; ++i) {
            m_outboxes[i]->push(chunk_buffer, reservations[i], location_id);
        }
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
        // TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
    }
    result.update(status, message_count, 1, latency, overhead);
}

} // namespace arrowengine
} // namespace jana


#endif //JANA2_ARROW_H
