
#ifndef JANA2_ARROW_H
#define JANA2_ARROW_H

#include <string>
#include <functional>
#include <vector>
#include <JANA/ArrowEngine/JMailbox.h>

namespace jana {
namespace arrowengine {

struct Arrow {
    std::string name;
    int chunk_size;
    bool is_finished = false;
    std::atomic_int active_upstream_count;
    std::vector<Arrow*> downstreams;

    virtual void execute() = 0;
    virtual ~Arrow() = default;
};

template <typename T>
struct ArrowWithBasicOutbox : public virtual Arrow {
    JMailbox<T>* m_outbox; // Outbox is owned by the downstream arrows
};

template <typename T>
struct ArrowWithBasicInbox : public virtual Arrow {
    JMailbox<T> m_inbox; // Arrow owns its inbox(es), but not its outbox(es)
};

template <typename T>
struct SourceArrow : public ArrowWithBasicOutbox<T> {
    using f_t = std::function<T(void)>;
    const f_t m_f;
    SourceArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T>
struct SinkArrow : public ArrowWithBasicInbox<T> {
    using f_t = std::function<void(T)>;
    const f_t m_f;
    SinkArrow(f_t f) : m_f(f) {}
    void execute() override {};
};

template <typename T, typename U>
struct StageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<U(T)>;
    const f_t m_f;
    StageArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T>
class BroadcastArrow : public ArrowWithBasicInbox<T> {
public:
    std::vector<JMailbox<T>*> m_outboxes;

    void execute() override {
        if (Arrow::is_finished) {
            // report.update_finished(); // TODO: Re-add this
            return;
        }
        int requested_count = Arrow::chunk_size;
        int dest_count = m_outboxes.size();
        std::vector<int> reservations(dest_count);
        for (int i=0; i<dest_count; ++i) {
            int reserved_count = m_outboxes[i]->reserve(requested_count);
            if (reserved_count < requested_count) requested_count = reserved_count;
            reservations[i] = reserved_count;
        }
        std::vector<T> chunk_buffer(requested_count); // TODO: Get rid of allocations
        if (requested_count != 0) {
            auto pop_result = ArrowWithBasicInbox<T>::m_inbox.pop(chunk_buffer, requested_count);
            if (pop_result == JMailbox<T>::Status::Ready) {
                // TODO: Set return status = success
            }
            else if ((pop_result == JMailbox<T>::Status::Empty) && (Arrow::active_upstream_count == 0)) {
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
    };
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
    JMailbox<T> m_inbox_1;
    JMailbox<U> m_inbox_2;
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

} // namespace arrowengine
} // namespace jana


#endif //JANA2_ARROW_H
