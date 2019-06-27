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

#include "DummyZmqSubscriber.h"
#include "DummyHit.h"

#include <JANA/JPerfUtils.h>

#include <iostream>

ZmqDummySubscriber::ZmqDummySubscriber(std::string socket_name,
                                       uint64_t delay_ms)

        : m_socket_name(socket_name)
        , m_context(1)
        , m_socket(m_context, ZMQ_SUB)
        , m_delay_ms(delay_ms) {

    m_socket.connect(m_socket_name);  // E.g. "tcp://*:5555"
    m_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // Subscribe to everything.
}

ZmqDummySubscriber::~ZmqDummySubscriber() {
    // socket closed by destructor
}


void ZmqDummySubscriber::loop() {

    zmq::message_t message(500); // What does this do about buffer size?
    Serializer<DummyHit> serializer;

    DummyHit x;
    size_t prev_id = 0;

    m_socket.recv(message);
    std::string text = std::string(message.data<char>());
    std::cout << text << std::endl;
    x = serializer.deserialize(text);
    prev_id = x.id;

    while (x.id < 10000) {
        m_socket.recv(message);
        std::string text = std::string(message.data<char>(), message.size());
        std::cout << text << std::endl;
        x = serializer.deserialize(text);
        if (x.id != prev_id + 1) {
            std::cout << "Dropped packet!" << std::endl;
            break;
        }
        prev_id = x.id;
        consume_cpu_ms(m_delay_ms, 0, false);
    }
}

int main() {
    ZmqDummySubscriber subscriber("tcp://127.0.0.1:5555", 1);
    subscriber.loop();
}