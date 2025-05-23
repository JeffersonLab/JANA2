
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _ZmqTransport_h_
#define _ZmqTransport_h_

#include <JANA/Services/JParameterManager.h>
#include <JANA/Streaming/JTransport.h>

#include <zmq.h>
#include <errno.h>

class ZmqTransport : public JTransport {

public:

    ZmqTransport(std::string socket_name, bool publish = false)
        : m_socket_name(socket_name)
        , m_publish(publish) {}

    ~ZmqTransport() {
        if (m_socket != nullptr) {
            zmq_close(m_socket);
        }
    }

    void initialize() override {

        m_context = zmq_ctx_new();
        int result;
        if (m_publish) {
            m_socket = zmq_socket(m_context, ZMQ_PUB);
            result = zmq_bind(m_socket, m_socket_name.c_str());
        }
        else {
            m_socket = zmq_socket(m_context, ZMQ_SUB);
            result = zmq_connect(m_socket, m_socket_name.c_str());
            zmq_setsockopt(m_socket, ZMQ_SUBSCRIBE, "", 0);  // Subscribe to everything
        }
        if (result == -1) {
            int errno_saved = errno;
            std::ostringstream os;
            os << "Unable to " << (m_publish ? "bind" : "connect") << " to zmq socket " << m_socket_name << ": ";
            switch (errno_saved) {
                case EINVAL: os << "Invalid endpoint"; break;
                case EPROTONOSUPPORT: os << "Transport protocol not supported"; break;
                case ENOCOMPATPROTO: os << "Transport protocol not compatible with socket type"; break;
                case EADDRINUSE: os << "Address in use"; break;
                case EADDRNOTAVAIL: os << "Address not available"; break;
                case ENODEV: os << "Address specifies nonexistent interface"; break;
                case ETERM: os << "Context associated with this socket was terminated"; break;
                case ENOTSOCK: os << "Invalid socket"; break;
                case EMTHREAD: os << "No I/O thread available"; break;
            }
            std::cout << os.str();
            throw JException(os.str());
        }

    };

    JTransport::Result send(const JMessage& src_msg) override {

        int rc = zmq_send(m_socket, src_msg.as_buffer(), src_msg.get_buffer_size(), 0);
        if (rc == -1) {
            return JTransport::Result::FAILURE;
        }
        return JTransport::Result::SUCCESS;
    }

    JTransport::Result receive(JMessage& dest_msg) override {

        int rc_length = zmq_recv(m_socket, dest_msg.as_buffer(), dest_msg.get_buffer_capacity(), ZMQ_DONTWAIT);
        if (rc_length == -1) {
            return JTransport::Result::TRY_AGAIN;
        }
        if (dest_msg.is_end_of_stream()) {
            zmq_close(m_socket);
            m_socket = nullptr;
            return JTransport::Result::FINISHED;
        }
        return JTransport::Result::SUCCESS;
    }

private:

    std::string m_socket_name = "tcp://127.0.0.1:5555";
    bool m_publish = false;
    void* m_context;
    void* m_socket;

};

#endif // _ZmqTransport_h_
