//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JTRIVIALWINDOW_H
#define JANA2_JTRIVIALWINDOW_H

#include <JANA/Streaming/JWindow.h>

/// JTrivialWindow emits a new JEvent for each JMessage it receives. This may be useful for simple
/// scenarios such as anomaly detection, or when events have already been built upstream so that
/// each JMessage corresponds to one event already.
template <typename T>
class JTrivialWindow : public JWindow<T> {
public:
    void pushMessage(T* message) final;
    bool pullEvent(JEvent& event) final;
private:
    std::deque<T*> m_pending_messages;
};

#endif //JANA2_JTRIVIALWINDOW_H
