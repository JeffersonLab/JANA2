//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JTRANSPORT_H
#define JANA2_JTRANSPORT_H

#include <JANA/Streaming/JMessage.h>

/// JTransport is a wrapper designed to integrate different messaging systems with JANA in a
/// lightweight and abstract way.
/// JMessages usually correspond to detector hits. They do not need to correspond to
/// individual messages on a queue: if detector hits are already aggregated into bundled into
/// 'hardware events', the JTransport implementation may separate and buffer JMessages as it sees fit.
struct JTransport {

    enum Result { SUCCESS, FAILURE, TRY_AGAIN, FINISHED };

    virtual void initialize() = 0;
    virtual Result send(const JMessage& src_msg) = 0;
    virtual Result receive(JMessage& dest_msg) = 0;
    virtual ~JTransport() = default;
};

#endif //JANA2_JTRANSPORT_H
