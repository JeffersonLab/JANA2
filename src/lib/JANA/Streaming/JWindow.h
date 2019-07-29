//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JWINDOW_H
#define JANA2_JWINDOW_H

#include <JANA/Streaming/JMessage.h>
#include <JANA/JEvent.h>

#include <queue>

/// JWindow is an abstract data structure for aggregating individual JMessages into a
/// single JEvent.  We generally assume that messages from any particular source arrive in-order, and
/// make no assumptions about ordering between different sources. We provide different implementations to fit
/// different use cases so that the user should rarely need to implement one of these by themselves.
/// The choice of JWindow determines how the time interval associated with each
/// JEvent is calculated, and furthermore is responsible for placing the correct JMessages into
/// the JEvent. As long as the time intervals do not overlap, this amounts to simple transferring
/// of ownership of a raw pointer. When JEvents intervals do overlap, which happens in the case of JSlidingWindow
/// and JMergeWindow, we have to decide whether to use shared ownership or to clone the offending data.
template <typename T>
struct JWindow {

    virtual ~JWindow() = default;
    virtual void pushMessage(T* message) = 0;
    virtual bool pullEvent(JEvent& event) = 0;

};



/// JFixedWindow partitions time into fixed, contiguous buckets, and emits a JEvent containing
/// all JMessages for all sources which fall into that bucket.
template <typename T>
class JFixedWindow : public JWindow<T> {
public:
    void pushMessage(T* message) final;
    bool pullEvent(JEvent& event) final;
private:
};

#endif //JANA2_JWINDOW_H
