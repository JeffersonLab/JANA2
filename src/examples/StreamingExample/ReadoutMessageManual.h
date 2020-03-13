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

/// If we know that both our producer and our consumer were compiled with the same
/// compiler and running on architectures with the same endianness, it is probably
/// safe (and certainly more convenient) to create JMessages like ReadoutMessageAuto.
/// However, if we can't make these assumptions, or if we want to explicitly control
/// where each individual byte goes, we design our JMessage like this instead:

class ReadoutMessageManual : public JEventMessage {

    /// Under the hood, ReadoutMessage is just a fixed-size buffer of char.
    /// We use getter and setter methods to create a 'view' into this buffer
    /// which automatically does the unpacking.
    static const size_t BUFFER_SIZE = 1024;
    char m_buffer[BUFFER_SIZE];
    size_t m_data_size;

public:
    /// Supplying the missing virtual functions needed to send the buffer over the wire

    inline char* as_buffer() override { return m_buffer; }
    inline size_t get_buffer_size() override { return BUFFER_SIZE; }
    inline size_t get_data_size() override { return BUFFER_SIZE; } // TODO: This is more complicated
    inline bool is_end_of_stream() override { return get_source_id() == 0 && get_message_id() == 0 && get_payload_size() == 0; }

    ReadoutMessage end_of_stream() override { return {}; }


    /// Exposing ReadoutMessage to us as a high-level object

    ReadoutMessage(uint32_t source_id = 0, uint32_t message_id = 0) {
        set_source_id(source_id);
        set_message_id(message_id);
    }

    inline friend std::ostream& operator<< (std::ostream& os, const ReadoutMessage& msg) {
        std::stringstream ss;
        ss << msg.message_id << ": ";
        for (int i=0; i<5 && i<N; ++i) {
            ss << msg.payload[i] << ", ";
        }
        ss << "...";
        os << ss.str();
        return os;
    }

    inline size_t get_event_number() override {
        return 0;
    }

    inline size_t get_run_number() override {
        return 0;
    }

};


#endif //JANA2_DETECTORMESSAGE_H
