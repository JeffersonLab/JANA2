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
#include <cstring>
#include <sstream>

#include <JANA/JException.h>
#include <JANA/JApplication.h>
#include <JANA/Streaming/JMessage.h>


/// INDRA_Message should be exactly the same as INDRA_Stream_Test's stream_buffer struct
struct INDRAMessage {

    uint32_t source_id = 0;
    uint32_t total_bytes;
    uint32_t payload_bytes;
    uint32_t compressed_bytes;
    uint32_t magic;
    uint32_t format_version;
    uint32_t flags;
    uint64_t record_counter;
    struct timespec timestamp;
    uint32_t payload[];
};


class DASEventMessage : public JEventMessage {

public:

    ////////////////////////////////////////////////////////////////////////////////////////
    /// DASEventMessage constructor/destructor
    ///
    /// These enforce the following invariant:
    ///  - A buffer of fixed capacity (determinable at startup time) is allocated
    ///  - Its total capacity is always known and never changes
    ///  - It is released when the message is destroyed

    ///  - The size of the valid data contained within is NOT protected, because it is a C-style array.

    explicit DASEventMessage(JApplication* app) {
        // TODO: Right now we have to default these in the InitPlugin, as otherwise they won't show up in the table
        //       Consider this use case in the future when we try to clean up how we get our parameters

        // Extract any parameters needed to figure out the buffer size

        m_sample_count = app->GetParameterValue<size_t>("toydet:nsamples");
        m_channel_count = app->GetParameterValue<size_t>("toydet:nchannels");

        // Allocate the buffer. The buffer size must be determinable at startup time and
        // constant for the life of the program. If we are a consumer, the buffer should be large
        // enough to accept any message emitted from our producer. This object won't ever reallocate the buffer.

        m_buffer_capacity = sizeof(INDRAMessage) + 5 * (m_sample_count * m_channel_count);
        m_buffer = new char[m_buffer_capacity];
    }

    ~DASEventMessage() override {
        delete[] m_buffer;
    }



    ////////////////////////////////////////////////////////////////////////////////////////
    /// Everything in this section is used by JStreamingEventSource in order to figure out
    /// how to emit the message as an Event.
    ///
    /// Exposing a byte array representation is necessary to acquire new messages from some
    /// arbitrary JTransport. We distinguish between the buffer size (bytes of memory reserved)
    /// and the data size (bytes of memory containing useful data). Buffer size is invariant;
    /// data size is not, and relies on the user correctly setting a payload_length parameter
    /// somewhere in the message.

    size_t get_event_number() const override {
        return as_indra_message()->record_counter;
    }

    size_t get_run_number() const override {
        return 1;
    }

    bool is_end_of_stream() const override {
        return as_indra_message()->flags == 1;
    }

    char* as_buffer() override { return m_buffer; }

    const char* as_buffer() const override { return m_buffer; }

    size_t get_buffer_capacity() const override { return m_buffer_capacity; }

    size_t get_buffer_size() const override { return sizeof(INDRAMessage) + as_indra_message()->payload_bytes*sizeof(uint32_t); }



    ////////////////////////////////////////////////////////////////////////////////////////
    /// The following setters are NOT required by JStreamingEventSource, but useful for writing producers.
    /// It is always advisable to put the code for the setters close to the code for the getters.

    void set_end_of_stream() {
        as_indra_message()->flags = 1;
    }

    void set_event_number(size_t event_number) {
        as_indra_message()->record_counter = event_number;
    }

    void set_run_number(size_t run_number) {
    }


    ////////////////////////////////////////////////////////////////////////////////////////
    /// Everything in this section is used by user-defined JFactories and JEventProcessors to access
    /// whatever data was sent. This is a 'view' into the buffer which you can define however you like.
    ///
    /// The 'simple' way to do this is to simply cast the buffer as the correct type (in this case, INDRAMessage.)
    /// The 'correct' way to to do this is to write getters which memcpy data out of the
    /// buffer from the correct offsets and convert the endianness appropriately.
    ///
    /// INDRA_Stream_Test defines the payload as a flexible array member of type uint32.
    /// For now, we would much rather access it as an array of char. We do NOT muck with the INDRAMessage type --
    /// if we define it differently from the producer code, the compiler might align/pad it differently,
    /// and all our data gets corrupted. Instead, we write a 'as_payload' method which reinterprets the 'payload'
    /// region of the buffer as a char*, and converts the sizes (measured in counts of uint32_t) to and from bytes.


    /// Grants read/write access to any INDRAMessage members directly
    INDRAMessage* as_indra_message() {
        return reinterpret_cast<INDRAMessage*>(m_buffer);
    }


    /// Grants read-only access to any INDRAMessage members directly
    const INDRAMessage* as_indra_message() const {
        return reinterpret_cast<INDRAMessage*>(m_buffer);
    }


    /// Grants read-only access to the message payload as a byte array, which we need because INDRAMessage uses uint32_t instead
    void as_payload(const char** payload, size_t* payload_bytes) const {

        *payload = m_buffer + sizeof(INDRAMessage);  // TODO: Verify this
        *payload_bytes = as_indra_message()->payload_bytes * sizeof(uint32_t);  // TODO: payload_bytes should be payload_length
    }


    /// Grants read/write access to the message payload as a byte array, which we need because INDRAMessage uses uint32_t instead
    void as_payload(char** payload, size_t* payload_bytes, size_t* payload_capacity) {

        *payload = m_buffer + sizeof(INDRAMessage);  // TODO: Verify this
        *payload_bytes = as_indra_message()->payload_bytes / sizeof(uint32_t);  // TODO: payload_bytes should be payload_length
        *payload_capacity = m_buffer_capacity - sizeof(INDRAMessage);
    }

    /// Sets a payload size measured in bytes, handling the conversion from uint32_t
    void set_payload_size(uint32_t payload_bytes) {
        as_indra_message()->payload_bytes = payload_bytes / sizeof(uint32_t);
    }

    /// Conveniently access the sample count associated with this message
    size_t get_sample_count() const { return m_sample_count; }

    /// Conveniently access the channel count associated with this message
    size_t get_channel_count() const { return m_channel_count; }



private:
    size_t m_sample_count;
    size_t m_channel_count;
    char* m_buffer;
    size_t m_buffer_capacity;
};


/// Conveniently print a one-line summary of any DASEventMessage for debugging
std::ostream& operator<< (std::ostream& os, const DASEventMessage& message) {
    std::stringstream ss;
    const char* payload;
    size_t length;
    message.as_payload(&payload, &length);

    ss << "Evt " << message.get_event_number() << ": ";
    for (int i=0; i<10 && i<length; ++i) {
        ss << payload[i] << ", ";
    }
    ss << "...";
    os << ss.str();
    return os;
}



#endif //JANA2_INDRAMESSAGE_H
