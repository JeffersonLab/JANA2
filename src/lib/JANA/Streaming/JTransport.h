//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JTRANSPORT_H
#define JANA2_JTRANSPORT_H

enum JTransportResult { SUCCESS, FAILURE, TRY_AGAIN, FINISHED };

/// JTransport is a wrapper designed to integrate different messaging systems with JANA in a
/// lightweight and abstract way.
/// JMessages usually correspond to detector hits. They do not need to correspond to
/// individual messages on a queue: if detector hits are already aggregated into bundled into
/// 'hardware events', the JTransport implementation may separate and buffer JMessages as it sees fit.
template <typename T>
struct JTransport {

    virtual void initialize() = 0;
    virtual JTransportResult send(const T& src_msg) = 0;
    virtual JTransportResult receive(T& dest_msg) = 0;
    virtual ~JTransport() = default;
};

#endif //JANA2_JTRANSPORT_H
