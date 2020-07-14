
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

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
