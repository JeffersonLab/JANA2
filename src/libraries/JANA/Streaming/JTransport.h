
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTRANSPORT_H
#define JANA2_JTRANSPORT_H

#include <JANA/Streaming/JMessage.h>

/// JTransport is a lightweight wrapper for integrating different messaging systems with JANA.

struct JTransport {

    enum Result { SUCCESS   /// send/recv succeeded
                , FAILURE   /// Not sure if we want this. Probably throw an exception instead.
                , TRY_AGAIN /// send/recv failed
                , FINISHED  /// Not sure if we want this. End-of-stream happens at a higher level.
    };


    /// Called exactly once before any calls to send or receive. Use for opening sockets, etc.
    /// We don't open sockets in the constructor because JANA expects multiple event sources, which
    /// might be constructed immediately but not used until much later, unnecessarily tying up resources.
    virtual void initialize() = 0;

    /// send should block until the underlying transport has successfully enqueued the message somewhere else.
    /// This allows the caller to reuse the same JMessage buffer immediately.
    virtual Result send(const JMessage& src_msg) = 0;

    /// receive should return as soon as the dest_msg has been written. If there are no messages waiting,
    /// receive should return TRY_AGAIN immediately instead of blocking.
    virtual Result receive(JMessage& dest_msg) = 0;

    /// It is reasonable to close sockets in the destructor, since:
    ///  a. The JTransport doesn't have an end-of-stream concept to hook a close() method to
    ///  b. The JStreamingEventSource owns the JTransport, so it can destroy it as soon as it is done with it
    virtual ~JTransport() = default;
};

#endif //JANA2_JTRANSPORT_H
