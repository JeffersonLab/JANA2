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
#include <JANA/Streaming/JMessage.h>
#include <JANA/JException.h>
#include <cstring>

struct ReadoutMessageAuto : public JMessage {

    static const size_t MAX_PAYLOAD_SIZE = 100;

    uint32_t source_id;
    uint32_t message_id;
    struct timespec timestamp;
    uint32_t payload_size = 0;
    float payload[MAX_PAYLOAD_SIZE];

public:
    /// JMessage requires that we expose this as a raw byte array

    inline char* as_buffer() override { return reinterpret_cast<char*>(this); }
    inline const char* as_buffer() const override { return reinterpret_cast<const char*>(this); }

    inline size_t get_buffer_size() const override { return sizeof(*this); }
    inline size_t get_data_size() const override { return sizeof(*this); } // TODO: If we make this more accurate, sending becomes cheaper
    inline bool is_end_of_stream() const override { return get_source_id() == 0 && message_id == 0 && payload_size == 0; }

    // We need an end-of-stream packet
    static ReadoutMessageAuto end_of_stream() { return {}; }

    ReadoutMessageAuto(uint32_t source_id = 0, uint32_t message_id = 0) : source_id(source_id), message_id(message_id) {

    }

    inline friend std::ostream& operator<< (std::ostream& os, const ReadoutMessageAuto& msg) {
        std::stringstream ss;
        ss << msg.message_id << ": ";
        for (int i = 0; i < msg.payload_size; ++i) {
            ss << msg.payload[i] << ", ";
        }
        os << ss.str();
        return os;
    }

    inline DetectorId get_source_id() const override {
        return source_id;
    }

    inline Timestamp get_timestamp() const override {
        return timestamp.tv_nsec;
    }

};


#endif //JANA2_DETECTORMESSAGE_H
