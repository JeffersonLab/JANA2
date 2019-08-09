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

#include <JANA/JParameterManager.h>
#include <JANA/Streaming/JTransport.h>

#include "zmq.hpp"


class ZmqTransport : public JTransport {

public:
    ZmqTransport(std::string socket_name, bool publish = false)
        : m_socket_name(socket_name)
        , m_context(1)
        , m_socket(m_context, publish ? zmq::socket_type::pub : zmq::socket_type::sub) {
    }

    void initialize() override {
        try {
            m_socket.connect(m_socket_name);
            m_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // Subscribe to everything.
        }
        catch (...) {
            throw JException("Unable to subscribe to zmq socket!");
        }
    };

    JTransport::Result send(const JMessage& src_msg) override {


        zmq::message_t message(src_msg.get_buffer_size());

        memcpy(message.data<char>(), src_msg.as_buffer(), src_msg.get_buffer_size());

        auto result = m_socket.send(message, zmq::send_flags::dontwait);

        if (!result.has_value()) {  // TODO: Not sure this actually does what I think it does
            return Result::TRY_AGAIN;
        }
        return Result::SUCCESS;
    }

    Result receive(JMessage& dest_msg) override {

        zmq::message_t message(dest_msg.get_buffer_size()); // TODO: This is wrong
        auto result = m_socket.recv(message, zmq::recv_flags::dontwait);

        if (!result.has_value()) {
            return Result::TRY_AGAIN;
        }

        memcpy(dest_msg.as_buffer(), message.data<char>(), message.size());

        std::stringstream ss;
        std::cout << ss.str();

        if (dest_msg.is_end_of_stream()) {
            return Result::FINISHED;
        }
        return Result::SUCCESS;
    }


private:
    std::string m_socket_name = "tcp://127.0.0.1:5555";
    JApplication* m_app;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
};


#endif //JANA2_ZMQDATASOURCE_H
