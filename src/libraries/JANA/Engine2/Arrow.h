
#ifndef JANA2_ARROW_H
#define JANA2_ARROW_H

class Arrow {
    std::string name;
    bool is_finished;
    const bool is_parallel;

    uint32_t thread_count;
    std::atomic_int active_upstreams;
    std::vector<Arrow*> downstreams;

    // TODO: Add backoff strategies, metrics, etc

    virtual void execute() = 0;
};

template <typename T>
class ArrowWithBasicOutbox : virtual Arrow {
protected:
    std::vector<JMailbox*> m_outboxes;
};

template <typename T>
class ArrowWithBasicInbox : virtual Arrow {
protected:
    JMailbox* m_inbox;
};

template <typename T>
class SourceArrow : public ArrowWithBasicOutbox<T> {
    using f_t = std::function<T(void)>;
    const f_t m_f;
public:
    SourceArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T>
class SinkArrow : public ArrowWithBasicInbox<T> {
    using f_t = std::function<void(T)>;
    const f_t m_f;
public:
    SinkArrow(f_t f) : m_f(f) {}
    void execute() override {};
};

template <typename T, typename U>
class StageArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<U(T)>;
    const f_t m_f;
public:
    StageArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

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

template <typename T, typename U>
class ScatterArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<std::pair<U,int>(T)>;
    const f_t m_f;
public:
    ScatterArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T, typename U>
class GatherArrow : public ArrowWithBasicOutbox<U> {
    std::vector<JMailbox<T>*> m_inboxes;
    using f_t = std::function<U(T,int)>;
    const f_t m_f;
public:
    GatherArrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T, typename U, typename V>
class Scatter2Arrow : public ArrowWithBasicInbox<T> {
    JMailbox<U>* m_outbox_1;
    JMailbox<V>* m_outbox_2;
    using f_t = std::function<std::variant<U,V>(T)>;
    const f_t m_f;
public:
    Scatter2Arrow(f_t f) : m_f(f) {};
    void execute() override {};
};

template <typename T, typename U, typename V>
class Gather2Arrow : public ArrowWithBasicOutbox<V> {
    JMailbox<T> m_inbox_1;
    JMailbox<U> m_inbox_2;
    using f_t = std::function<V(T)>;
    using g_t = std::function<V(U)>;
    const f_t m_f;
    const g_t m_g;
public:
    Gather2Arrow(f_t f, g_t g) : m_f(f), m_g(g) {};
    void execute() override {};
};

template <typename T>
void attach(ArrowWithBasicOutbox<T>& upstream, ArrowWithBasicInbox<T>& downstream) {
    upstream.downstreams.push_back(&downstream);
    upstream.m_outboxes.push_back(&downstream.m_inbox);
    downstream.active_upstreams += 1;
}

template <typename T, typename U, typename V>
void attach(Scatter2Arrow<T,U,V>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.downstreams.push_back(&downstream);
    upstream.m_outboxes.push_back(&downstream.m_inbox);
    downstream.active_upstreams += 1;
}

template <typename T>
void attach(ArrowWithBasicOutbox<T>& upstream, GatherArrow<T>& downstream) {

}

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T>& upstream, GatherArrow2<T,U,V>& downstream) {

}

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T> upstream, GatherArrow2<T,U,V> downstream) {

}






#endif //JANA2_ARROW_H
