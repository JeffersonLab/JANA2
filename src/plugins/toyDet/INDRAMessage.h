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

#ifndef JANA2_INDRAMESSAGE_H
#define JANA2_INDRAMESSAGE_H


#include <string>
#include <chrono>
#include "../zmq2jana/internals/JSampleSource.h"
#include <JANA/JException.h>
#include <cstring>
#include <sstream>



/// INDRA_Message should be the same as INDRA_Stream_Test's stream_buffer struct,
/// only we can choose our max payload size and type as template parameters, which
/// is extremely convenient.
template <typename T, unsigned int N>
struct INDRAMessage {

    uint32_t source_id = 0;
    uint32_t total_length;
    uint32_t payload_length;
    uint32_t compressed_length;
    uint32_t magic;
    uint32_t format_version;
    uint64_t record_counter;
    struct timespec timestamp;
    T payload[N];


    /// Zero-arg ctor needed so that we can automatically allocate a buffer that will get filled later
    /// using memcpy, in lieu of proper deserialization
    INDRAMessage() {}

    /// Make it convenient to create one of these things.
    /// Most fields are not used and therefore get defaulted, although
    /// because this is a struct you are free to change them at any point.
    INDRAMessage(uint64_t event_number, uint32_t channel_number, std::vector<T> data) {

        source_id = channel_number;
        total_length = sizeof(INDRAMessage);
        payload_length = static_cast<uint32_t>(data.size() * sizeof(T));
        compressed_length = payload_length;
        magic = 2227;
        format_version = 0;
        record_counter = event_number;
        clock_gettime(CLOCK_MONOTONIC, &timestamp);

        if (data.size() > N) {
            throw std::runtime_error("Event data is too large for INDRAMessage payload");
        }
        for (int i=0; i<data.size(); ++i) {
            payload[i] = data[i];
        }
    }

    inline friend std::ostream& operator<< (std::ostream& os, const INDRAMessage& msg) {
        std::stringstream ss;
        ss << "Evt " << msg.record_counter << ": Chan " << msg.source_id << ": ";
        for (int i=0; i<5 && i<N; ++i) {
            ss << msg.payload[i] << ", ";
        }
        ss << "...";
        os << ss.str();
        return os;
    }

    /// This is actually a free function, at least for now

    inline friend DetectorId get_detector_id(const INDRAMessage& m) {
        return std::to_string(m.payload.source_id);
    }

    /// This is actually a free function, at least for now

    inline friend Timestamp get_timestamp(const INDRAMessage& m) {
        return m.payload.timestamp.tv_nsec;
    }

    /// This is actually a free function, at least for now

    inline friend bool end_of_stream(const INDRAMessage& m) {
        return m.source_id == 0 && m.record_counter == 0;
    }

    // TODO: Rethink having this here (also obviously rethink doing a memcpy at all)
    inline friend void emplace(INDRAMessage* destination, char* source, size_t bytes) {
        memcpy(destination, source, bytes);
    }

};

/// We use a type alias in order to not have to parameterize INDRAMessages everywhere
using ToyDetMessage = INDRAMessage<double, 2048>;


#endif //JANA2_INDRAMESSAGE_H
