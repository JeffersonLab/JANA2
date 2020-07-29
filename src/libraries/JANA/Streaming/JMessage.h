
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JMESSAGE_H
#define JANA2_JMESSAGE_H

#include <JANA/JObject.h>

using DetectorId = uint64_t;
using Timestamp = uint64_t;


/// JMessage is an interface for data that can be streamed using JTransports.
///
/// The basic goal is to have an object wrapper around an old-fashioned `char*` buffer
/// which can be created automatically on either the stack or the heap. Most importantly,
/// it encapsulates the mess arising from variable message lengths. A common pattern
/// for solving this problem in C is to use flexible array members. Do NOT do this here,
/// as this causes a variety of problems when it comes to type safety and memory safety,
/// and it is not even part of any C++ standard. Instead, agree upon a max message size
/// (which your transport needs anyway), and declare an array of that length. The
/// array length (a.k.a. 'capacity') should be a compile-time constant, and the length
/// of the data contained (a.k.a. 'size') should be tracked in a separate variable.
///
/// In addition to the pure virtual methods listed below, conforming implementations need
/// a zero-argument constructor.

struct JMessage {

    /// Expose the underlying buffer via a raw pointer
    /// \return A raw pointer to the buffer
    virtual char* as_buffer() = 0;

    /// Expose the underlying buffer via a raw pointer
    /// \return A raw pointer to the buffer
    virtual const char* as_buffer() const = 0;

    /// Determine the length of the buffer itself, a.k.a. the max number of bytes that can be written.
    /// \return The number of bytes allocated for the buffer
    virtual size_t get_buffer_capacity() const = 0;

    /// Determine the length of the buffer's contents, a.k.a. the number of bytes which ought to be read
    /// \return The number of bytes of meaningful data inside the buffer
    virtual size_t get_buffer_size() const { return get_buffer_capacity(); }

    /// Determine whether this is the last message.
    /// An end-of-stream message is assumed to also contain a meaningful payload. It is not empty like EOF.
    /// TODO: Figure out best way to handle empty end-of-stream as well as other control signals such as change-run.
    /// \return Whether this is the last message to expect from the producer
    virtual bool is_end_of_stream() const = 0;
};


/// A JEventMessage is an interface used by JTransport for streaming individual events.
///
/// In other words, this assumes that event building is done upstream. Each JEventMessage will be inserted
/// into its own brand new JEvent just like any other JObject. The implementor is responsible for figuring out
/// the event and run numbers. The code for parsing the message payload could either be expressed as getter methods
/// on the JEventMessage implementation, or in its own JFactory. If the message format is the same as the in-memory
/// representation (non-portable but fast), the JEventMessage can be defined as a plain-old-data struct and the `char*`
/// buffer obtained via `reinterpret_cast`.

struct JEventMessage : public JMessage, public JObject {

    /// Determine what the event number should be for the JEvent that gets emitted for this message. This information
    /// should be available from the message payload. If not, return zero, in which case the JStreamingEventSource
    /// will issue an event number automatically.
    virtual size_t get_event_number() const = 0;

    /// Determine what the run number should be for the JEvent that gets emitted for this message. This information
    /// should be available from the message payload. If not, return zero, in which case the JStreamingEventSource
    /// will use the last known run number.
    virtual size_t get_run_number() const = 0;
};


/// A JHitMessage is an interface used by JTransport for streaming detector hits.
///
/// Either we do event building ourselves using JEventBuilder, or we hydrate an existing event using JDiscreteJoin.
///
/// These methods extract the information necessary to figure out which JEvent this Hit belongs to,
/// and also whether we've received data from all detectors needed before emitting a new Event.
/// This works for both discrete-in-time data and continuous-in-time data.

struct JHitMessage : public JMessage {

    /// Extract the detector ID from the message payload. This is mandatory.
    virtual DetectorId get_source_id() const = 0;

    /// Extract the detector ID from the message payload. This is mandatory.
    virtual Timestamp get_timestamp() const = 0;
};

#endif //JANA2_JMESSAGE_H
