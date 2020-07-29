
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_DETECTORMESSAGE_H
#define JANA2_DETECTORMESSAGE_H

#include <string>
#include <chrono>
#include <JANA/Streaming/JMessage.h>
#include <JANA/JException.h>
#include <JANA/JException.h>
#include <cstring>

struct ReadoutMessageAuto : public JEventMessage {

    static const size_t MAX_PAYLOAD_SIZE = 100;

    uint32_t run_number;
    uint32_t event_number;
    uint32_t payload_size = 0;
    float payload[MAX_PAYLOAD_SIZE];

public:
    /// JMessage requires that we expose this as a raw byte array

    inline char* as_buffer() override { return reinterpret_cast<char*>(this); }
    inline const char* as_buffer() const override { return reinterpret_cast<const char*>(this); }

    inline size_t get_buffer_capacity() const override { return sizeof(*this); }
    inline bool is_end_of_stream() const override { return event_number == 0 && run_number == 0 && payload_size == 0; }

    inline void set_end_of_stream() {
        event_number = 0;
        run_number = 0;
        payload_size = 0;
    }

    inline size_t get_event_number() const override { return event_number; }
    inline size_t get_run_number() const override { return run_number; }

    ReadoutMessageAuto(JApplication* app) {
    }

    inline friend std::ostream& operator<< (std::ostream& os, const ReadoutMessageAuto& msg) {
        std::stringstream ss;
        ss << msg.event_number << ": ";
        for (size_t i = 0; i < msg.payload_size; ++i) {
            ss << msg.payload[i] << ", ";
        }
        os << ss.str();
        return os;
    }

};


#endif //JANA2_DETECTORMESSAGE_H
