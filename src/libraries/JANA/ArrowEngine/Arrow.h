
#ifndef JANA2_ARROW_H
#define JANA2_ARROW_H

#include <string>
#include <functional>
#include <vector>
#include <JANA/ArrowEngine/Mailbox.h>

namespace jana {
namespace arrowengine {

struct Arrow {
    std::string name;
    int chunk_size = 10;
    bool is_finished = false;
    bool is_parallel;
    std::atomic_int active_upstream_count;
    std::vector<Arrow*> downstreams;

    virtual void execute() = 0;
    Arrow() = delete;
    Arrow(std::string name, bool is_parallel) : name(name), is_parallel(is_parallel) {
        std::cout << "Calling Arrow ctor with name=" << name << std::endl;
    };
    virtual ~Arrow() = default;
};

template <typename T>
struct ArrowWithBasicOutbox : public virtual Arrow {
    Mailbox<T>* m_outbox; // Outbox is owned by the downstream arrows
    ArrowWithBasicOutbox() {
        std::cout << "Constructing ArrowWithBasicOutbox" << std::endl;
    };
};


struct KMailbox {
    int x;
    KMailbox() : x(22) {
        std::cout << "Cosntructing kmailbox" << std::endl;
    }
    KMailbox(int x) : x(x) {
        std::cout << "Cosntructing kmailbox with x=" << x << std::endl;
    }
};

template <typename T>
struct ArrowWithBasicInbox : public virtual Arrow {
    Mailbox<T> m_inbox; // Arrow owns its inbox(es), but not its outbox(es)
    KMailbox m_dummy;
    ArrowWithBasicInbox()
    : m_inbox(10,1,false), m_dummy(23) {}
};

template <typename T>
struct SourceArrow : public ArrowWithBasicOutbox<T> {
    using f_t = std::function<T()>;
    const f_t m_f;
    SourceArrow(std::string name, f_t f, bool is_parallel=false)
    : Arrow(name, is_parallel), ArrowWithBasicOutbox<T>(), m_f(f)
    {
        std::cout << "Constructing SourceArrow" << std::endl;
    };
    void execute() override;
};

template <typename T>
struct SinkArrow : public ArrowWithBasicInbox<T> {
    using f_t = std::function<void(T)>;
    const f_t m_f;
    SinkArrow(std::string name, f_t f, bool is_parallel)
    : Arrow(name, is_parallel), m_f(f) {

        std::cout << "Constructing SinkArrow" << std::endl;
    }
    void execute() override;
};

template <typename T, typename U>
struct StageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<U(T)>;
    const f_t m_f;
    StageArrow(std::string name, f_t f, bool is_parallel)
    : Arrow(name, is_parallel)
    , m_f(f) {
        std::cout << "Constructing StageArrow" << std::endl;
    };
    void execute() override;
};

template <typename T>
class BroadcastArrow : public ArrowWithBasicInbox<T> {
public:
    std::vector<Mailbox<T>*> m_outboxes;
    BroadcastArrow(std::string name, bool is_parallel)
    : Arrow(name, is_parallel) {
        std::cout << "Constructing BroadcastArrow" << std::endl;
    }
    void execute() override;
};

#if __cpp_lib_optional
template <typename T, typename U, typename A>
class MultiStageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<A(T)>;
    using g_t = std::function<std::optional<U>(A)>;
    const f_t m_f;
    const g_t m_g;
public:
    MultiStageArrow(f_t f, g_t g) : m_f(f), m_g(g) {};
    void execute() override {};
};
#endif

template <typename T, typename U>
struct ScatterArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<std::pair<U,int>(T)>;
    const f_t m_f;
    ScatterArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

#if __cpp_lib_variant
template <typename T, typename U, typename V>
class SplitArrow : public ArrowWithBasicInbox<T> {
    JMailbox<U>* m_outbox_1;
    JMailbox<V>* m_outbox_2;
    using f_t = std::function<std::variant<U,V>(T)>;
    const f_t m_f;
public:
    SplitArrow(f_t f) : m_f(f) {};
    void execute() override {};
};
#endif

template <typename T, typename U, typename V>
struct MergeArrow : public ArrowWithBasicOutbox<V> {
    Mailbox<T> m_inbox_1;
    Mailbox<U> m_inbox_2;
    using f_t = std::function<V(T)>;
    using g_t = std::function<V(U)>;
    const f_t m_f;
    const g_t m_g;
public:
    MergeArrow(f_t f, g_t g) : m_f(f), m_g(g) {};
    void execute() override {};
};

template <typename T>
void attach(ArrowWithBasicOutbox<T>& upstream, ArrowWithBasicInbox<T>& downstream) {
    upstream.m_outbox = &downstream.m_inbox;
    downstream.active_upstream_count += 1;
    upstream.downstreams.push_back(&downstream);
}

template <typename T, typename U, typename V>
void attach(BroadcastArrow<T>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.m_outboxes.push_back(&downstream.m_inbox);
    downstream.active_upstreams += 1;
    upstream.downstreams.push_back(&downstream);
}

#if __cpp_lib_variant
template <typename T, typename U, typename V>
void attach(SplitArrow<T,U,V>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.m_outbox_1 = &downstream.m_inbox;
    downstream.active_upstreams += 1;
    upstream.downstreams.push_back(&downstream);
}
#endif

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

template <typename T>
void SourceArrow<T>::execute() {

    if (Arrow::is_finished) {
        // report.update_finished(); // TODO: Re-add this
        return;
    }
    int requested_count = Arrow::chunk_size;
    std::vector<T> chunk_buffer(requested_count); // TODO: Get rid of allocations
    int reserved_count = this->m_outbox->reserve(requested_count);

    if (reserved_count != 0) {
        for (int i=0; i<reserved_count; ++i) {
            chunk_buffer[i] = m_f();
            // TODO: Modify m_f to return status code, only populate chunk buffer with valid T
        }

        // We have to return our reservation regardless of whether our pop succeeded
        this->m_outbox->push(chunk_buffer, reserved_count);

        if (false) { // If lambda returns keep going
            // TODO: set return status = keep going
        }
        else if (false) { // If lambda returns try again later
            // TODO: set return status = try again later
        }
        else if (false) {  // TODO: if lambda returns status::finished
            // TODO: set return status = finished
            Arrow::is_finished = true;
            for (auto downstream : Arrow::downstreams) {
                downstream->active_upstream_count -= 1;
            }
        }
    }
    else {
        // Set return status = downstream full, try again later
    }
}

template <typename T>
void SinkArrow<T>::execute() {

    if (Arrow::is_finished) {
        // report.update_finished(); // TODO: Re-add this
        return;
    }
    std::vector<T> chunk_buffer(Arrow::chunk_size); // TODO: Get rid of allocations

    auto pop_result = this->m_inbox.pop(chunk_buffer, Arrow::chunk_size);

    // TODO: Timing information
    for (auto t : chunk_buffer) {
        m_f(t);
    }

    if (pop_result == Mailbox<T>::Status::Ready) {
        // TODO: Set return status = success
    }
    else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
        // TODO: Set return status = finished
        Arrow::is_finished = true;
        for (auto downstream : Arrow::downstreams) {
            downstream->active_upstream_count -= 1;
        }
    }
    else {
        // TODO: Set return status = upstream empty
    }
}

template <typename T, typename U>
void StageArrow<T,U>::execute() {

    if (Arrow::is_finished) {
        // report.update_finished(); // TODO: Re-add this
        return;
    }
    int requested_count = Arrow::chunk_size;
    std::vector<T> input_chunk_buffer(requested_count); // TODO: Get rid of allocations
    std::vector<U> output_chunk_buffer(requested_count); // TODO: Get rid of allocations

    int reserved_count = this->m_outbox->reserve(requested_count);
    if (reserved_count != 0) {
        auto pop_result = this->m_inbox.pop(input_chunk_buffer, this->chunk_size);

        // TODO: Timing information
        auto item_count = input_chunk_buffer.size();
        for (int i = 0; i<item_count; ++i) {
            output_chunk_buffer[i] = m_f(input_chunk_buffer[i]);
        }

        // We have to return our reservation regardless of whether our pop succeeded
        this->m_outbox->push(output_chunk_buffer, reserved_count);

        if (pop_result == Mailbox<T>::Status::Ready) {
            // TODO: Set return status = success
        }
        else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
            // TODO: Set return status = finished
            Arrow::is_finished = true;
            for (auto downstream : Arrow::downstreams) {
                downstream->active_upstream_count -= 1;
            }
        }
        else {
            // TODO: Set return status = upstream empty
        }
    }
    else {
        // Set return status = upstream empty
    }
}


template <typename T>
void BroadcastArrow<T>::execute() {

    if (Arrow::is_finished) {
        // report.update_finished(); // TODO: Re-add this
        return;
    }
    int requested_count = this->chunk_size;
    int dest_count = m_outboxes.size();
    std::vector<int> reservations(dest_count);
    for (int i=0; i<dest_count; ++i) {
        int reserved_count = m_outboxes[i]->reserve(requested_count);
        if (reserved_count < requested_count) requested_count = reserved_count;
        reservations[i] = reserved_count;
    }
    std::vector<T> chunk_buffer(requested_count); // TODO: Get rid of allocations
    if (requested_count != 0) {
        auto pop_result = this->m_inbox.pop(chunk_buffer, requested_count);
        if (pop_result == Mailbox<T>::Status::Ready) {
            // TODO: Set return status = success
        }
        else if ((pop_result == Mailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
            // TODO: Set return status = finished
            Arrow::is_finished = true;
            for (auto downstream : Arrow::downstreams) {
                downstream->active_upstream_count -= 1;
            }
        }
        else {
            // TODO: Set return status = tryagainlater
        }
    }
    for (int i=0; i<dest_count; ++i) {
        m_outboxes[i]->push(chunk_buffer, reservations[i]);
    }

}

} // namespace arrowengine
} // namespace jana


#endif //JANA2_ARROW_H
