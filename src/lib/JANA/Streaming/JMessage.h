//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JMESSAGE_H
#define JANA2_JMESSAGE_H

#include <JANA/JObject.h>

using DetectorId = uint64_t;
using Timestamp = uint64_t;


/// JMessage is an abstract base class for data that can be streamed using JTransport's.
/// In addition to the methods required below, it needs a zero-argument constructor and a
/// constant sizeof(T) which is known at compile-time. In other words, we don't
/// allow flexible array members at the end -- use an array of fixed size instead,
/// and override get_data_size() to avoid sending over the garbage bytes.

struct JMessage {

    virtual char* as_buffer() = 0;
    virtual const char* as_buffer() const = 0;
    virtual size_t get_buffer_capacity() const = 0;
    virtual bool is_end_of_stream() const = 0;
    virtual size_t get_buffer_size() const { return get_buffer_capacity(); }
};


/// A JEventMessage represents a single event, meant to be used when the event building
/// is done upstream. Each JEventMessage will be inserted into its own brand new JEvent
/// just like any other JObject. Parsing can either be done by additional methods on
/// the JEventMessage, or by one or more JFactories.

struct JEventMessage : public JMessage, public JObject {

    virtual size_t get_event_number() const = 0;
    virtual size_t get_run_number() const = 0;
};


/// A JHitMessage represents a single detector hit.
///
/// Either we do event building ourselves using JEventBuilder, or we hydrate an existing event using JDiscreteJoin.
///
/// These methods extract the information necessary to figure out which JEvent this Hit belongs to,
/// and also whether we've received data from all detectors needed before emitting a new Event.
/// This works for both discrete-in-time data and continuous-in-time data.

struct JHitMessage : public JMessage {

    virtual DetectorId get_source_id() const = 0;
    virtual Timestamp get_timestamp() const = 0;
};

#endif //JANA2_JMESSAGE_H
