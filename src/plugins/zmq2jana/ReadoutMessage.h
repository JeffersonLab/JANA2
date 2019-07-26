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

#ifndef JANA2_DETECTORMESSAGE_H
#define JANA2_DETECTORMESSAGE_H

#include <string>
#include <chrono>
#include "internals/JSampleSource.h"
#include <JANA/JException.h>
#include <cstring>


/// ReadoutMessage should be the same as INDRA_Stream_Test's stream_buffer struct.
template <unsigned int N>
struct ReadoutMessage {

    uint32_t source_id;
    uint32_t total_length;
    uint32_t payload_length;
    uint32_t compressed_length;
    uint32_t magic;
    uint32_t format_version;
    uint64_t record_counter;
    struct timespec timestamp;
    uint32_t payload[N];


    template <typename T>
    T* get_payload() {
        return reinterpret_cast<T*>(payload);
    }

    inline friend std::ostream& operator<< (std::ostream& os, const ReadoutMessage& msg) {
        std::stringstream ss;
        ss << msg.record_counter << ": " << reinterpret_cast<const float&>(msg.payload[0]) << ", ... " << reinterpret_cast<const float&>(msg.payload[1]);
        os << ss.str();
        return os;
    }

};


template <unsigned int N>
inline DetectorId get_detector_id(const ReadoutMessage<N>& m) {
    return std::to_string(m.payload.source_id);
}

template <unsigned int N>
inline Timestamp get_timestamp(const ReadoutMessage<N>& m) {
    return m.payload.timestamp.tv_nsec;
}

template <unsigned int N>
inline bool end_of_stream(const ReadoutMessage<N>& m) {
    return m.source_id == 0 && m.record_counter == 0;
}

template <unsigned int N>
inline void emplace(ReadoutMessage<N>* destination, char* source, size_t bytes) {
    memcpy(destination, source, bytes);
}


#endif //JANA2_DETECTORMESSAGE_H
