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
#include "../zmq2jana/ReadoutMessage.h"

#include <JANA/JPerfUtils.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <sstream>

ZmqDummyPublisher::ZmqDummyPublisher(std::string file_name,
                                     std::string socket_name,
                                     uint64_t delay_ms)

                                     : m_file_name(file_name)
                                     , m_socket_name(socket_name)
                                     , m_delay_ms(delay_ms)
                                     , m_context(1)
                                     , m_socket(m_context, ZMQ_PUB)
{
    m_socket.bind(m_socket_name);  // E.g. "tcp://*:5555"
}

ZmqDummyPublisher::~ZmqDummyPublisher() = default;



void ZmqDummyPublisher::loop() {

    std::ifstream file_stream;
    std::string line;
    std::stringstream line_stream;
    std::vector<double> event;

    size_t line_count = 0;
    size_t event_count = 0;

    while (std::getline(file_stream, line)) {

        if (line[0] != '#' ) {
            line_count += 1;
            line_stream.str(line);
            std::string value;

            while (std::getline(line_stream, value, ' ')) {
                event.push_back(std::stod(value));
            }
        }
        if (line_count == 19) {
            line_count = 0;

            struct timespec timestamp;
            clock_gettime(CLOCK_MONOTONIC, &timestamp);

            ReadoutMessage<100> message {
                    .source_id = 22,
                    .total_length = sizeof(ReadoutMessage<4>),
                    .payload_length = 4,
                    .compressed_length = 4,
                    .magic = 618,
                    .format_version = 0,
                    .record_counter = event_count++,
                    .timestamp = timestamp };

            auto* payload = message.get_payload<double>();
            for (size_t i=0; i<event.size(); ++i) {
                payload[i] = event[i];
            }

            m_socket.send(zmq::buffer(&message, sizeof(ReadoutMessage<100>)), zmq::send_flags::dontwait);
            std::cout << "Send: " << message << " (" << sizeof(ReadoutMessage<100>) << " bytes)" << std::endl;
            consume_cpu_ms(m_delay_ms, 0, false);

        }
    }
}


