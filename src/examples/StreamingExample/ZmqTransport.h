//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.

// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.

// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#ifndef JANA2_ZMQDATASOURCE_H
#define JANA2_ZMQDATASOURCE_H

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


#endif //JANA2_ZMQDATASOURCE_H
