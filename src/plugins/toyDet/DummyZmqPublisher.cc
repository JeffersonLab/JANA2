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
#include "../../examples/JExample7/ReadoutMessage.h"
#include "INDRAMessage.h"

#include <JANA/JPerfUtils.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <sstream>

ZmqDummyPublisher::ZmqDummyPublisher(std::string file_name,
                                     std::string socket_name,
                                     uint64_t delay_ms,
                                     uint32_t total_channels,
                                     uint32_t total_sources)

                                     : m_file_name(file_name)
                                     , m_socket_name(socket_name)
                                     , m_delay_ms(delay_ms)
                                     , m_total_channels(total_channels)
                                     , m_total_sources(total_sources)
                                     , m_context(1)
                                     , m_socket(m_context, ZMQ_PUB)
{
    m_socket.bind(m_socket_name);  // E.g. "tcp://*:5555"
}

ZmqDummyPublisher::~ZmqDummyPublisher() = default;



void ZmqDummyPublisher::loop() {

    std::cout << "Starting producer loop" << std::endl;

    size_t event_id = 1;
    uint32_t channel_id = 1;
    uint32_t source_id = 1;

    std::ifstream file_stream(m_file_name);
    std::string line;
    std::vector<double> data;

    while (std::getline(file_stream, line)) {

        // Skip comment lines
        if (line[0] == '#' || line[0] == '@') continue;

        // Parse line as sequence of doubles
        std::stringstream line_stream(line);
        std::string value;
        while (std::getline(line_stream, value, ' ')) {
            data.push_back(std::stod(value));
        }

        // Append all sources to data buffer
        source_id += 1;
        if (source_id <= m_total_sources) continue;

        // Send line as message over ZMQ
        auto message = ToyDetMessage(event_id, channel_id, data);
        m_socket.send(zmq::buffer(&message, sizeof(ToyDetMessage)), zmq::send_flags::dontwait);
        std::cout << "Send: " << message << " (" << sizeof(ToyDetMessage) << " bytes)" << std::endl;
        consume_cpu_ms(m_delay_ms, 0, false);
        data.clear();

        // Update source and channel ids
        source_id = 1;
        channel_id += 1;
        if (channel_id > m_total_channels) {
            event_id += 1;
            channel_id = 1;
        }
    }

    // Send an end-of-stream message
    auto message = ToyDetMessage(0, 0, data);
    m_socket.send(zmq::buffer(&message, sizeof(ToyDetMessage)), zmq::send_flags::dontwait);
    std::cout << "Send: " << message << " (" << sizeof(ToyDetMessage) << " bytes)" << std::endl;
}


