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


#include "DummyZmqPublisher.h"
#include "ReadoutMessage.h"

#include <JANA/JPerfUtils.h>

#include <iostream>
#include <thread>

ZmqDummyPublisher::ZmqDummyPublisher(std::string socket_name,
                                     std::string sensor_name,
                                     size_t samples_avg,
                                     size_t samples_spread,
                                     uint64_t delay_ms)

                                     : m_socket_name(socket_name)
                                     , m_sensor_name(sensor_name)
                                     , m_samples_avg(samples_avg)
                                     , m_samples_spread(samples_spread)
                                     , m_delay_ms(delay_ms)
                                     , m_context(1)
                                     , m_socket(m_context, ZMQ_PUB)
                                     , m_prev_time(0.0) {

    m_socket.bind(m_socket_name);  // E.g. "tcp://*:5555"
}

ZmqDummyPublisher::~ZmqDummyPublisher() {
    // socket closed by destructor
}

void ZmqDummyPublisher::publish(size_t nitems) {

    size_t counter = 0;

    while (counter < nitems) {

        struct timespec timestamp;
        clock_gettime(CLOCK_MONOTONIC, &timestamp);

        float V = randfloat(0,1);
        float x = randfloat(-100, 100);
        float y = randfloat(-100, 100);
        float z = randfloat(-100, 100);

        ReadoutMessage<float, 4> message(22, counter, {V, x, y, z});

        m_socket.send(zmq::buffer(&message, message.get_buffer_size()), zmq::send_flags::dontwait);
        std::cout << "Send: " << message << " (" << message.get_buffer_size() << " bytes)" << std::endl;
        consume_cpu_ms(m_delay_ms, 0, false);
    }

    // Send end-of-stream message
    auto eos = ReadoutMessage<float, 4>::end_of_stream();
    m_socket.send(zmq::buffer(&eos, eos.get_buffer_size()));
}



