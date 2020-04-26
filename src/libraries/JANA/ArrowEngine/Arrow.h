
#ifndef JANA2_ARROW_H
#define JANA2_ARROW_H

#include <string>
#include <functional>
#include <vector>
#include <JANA/ArrowEngine/JMailbox.h>

namespace jana {
namespace arrowengine {

class Arrow {
    std::string name;
    std::atomic_int active_upstream_count;
    std::vector<Arrow*> downstreams;

public:
    virtual void execute() = 0;
};

template <typename T>
class ArrowWithBasicOutbox : virtual Arrow {
protected:
    std::vector<JMailbox<T>*> m_outboxes; // Outboxes are owned by the downstream arrows
};

template <typename T>
class ArrowWithBasicInbox : virtual Arrow {
protected:
    JMailbox<T> m_inbox; // Arrow owns its inbox(es), but not its outbox(es)
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
class ScatterArrow : public ArrowWithBasicInbox<T>, public ArrowWithBasicOutbox<U> {
    using f_t = std::function<std::pair<U,int>(T)>;
    const f_t m_f;
public:
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
class MergeArrow : public ArrowWithBasicOutbox<V> {
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
    upstream.downstreams.push_back(&downstream);
    upstream.m_outboxes.push_back(&downstream.m_inbox);
    downstream.active_upstream_count += 1;
}

#if __cpp_lib_variant
template <typename T, typename U, typename V>
void attach(SplitArrow<T,U,V>& upstream, ArrowWithBasicInbox<U>& downstream) {
    upstream.m_outbox_1 = &downstream.m_inbox;
    upstream.downstreams.push_back(&downstream);
    downstream.active_upstreams += 1;
}
#endif

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T>& upstream, MergeArrow<T,U,V>& downstream) {

}

template <typename T, typename U, typename V>
void attach(ArrowWithBasicOutbox<T> upstream, MergeArrow<T,U,V> downstream) {

}

} // namespace arrowengine
} // namespace jana




#endif //JANA2_ARROW_H
