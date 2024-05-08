
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once
#include <JANA/Streaming/JWindow.h>

/// JMergeWindow 'hydrates' an existing JEvent by appending any JMessages that fall into its
/// pre-existing time interval. This is unlike the other JWindows, which assume the JEvent
/// contains no JObjects and has no associated time interval. It should be used downstream
/// of a TrivialWindow/FixedWindow/SessionWindow, e.g. for level 2 triggers,
/// EPICS data, or calibration constants. It should probably not be public-facing.
template <typename T>
class JMergeWindow : public JWindow<T> {
public:
    void pushMessage(T* message) final;
    bool pullEvent(JEvent& event) final;
private:
};

