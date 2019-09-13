//
// Author: Nathan Brei
//

#ifndef _INDRAMessage_h_
#define _INDRAMessage_h_

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
    uint32_t total_bytes{};
    uint32_t payload_bytes{};
    uint32_t compressed_bytes{};
    uint32_t magic{};
    uint32_t format_version{};
    uint32_t flags{};
    uint64_t record_counter{};
    struct timespec timestamp{};
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

    explicit DASEventMessage(JApplication *app) {

        // TODO: Right now we have to default these in the InitPlugin, as otherwise they won't show up in the table
        //       Consider this use case in the future when we try to clean up how we get our parameters

        // Extract any parameters needed to figure out the buffer size
        m_sample_count  = app->GetParameterValue<size_t>("toydet:nsamples");
        m_channel_count = app->GetParameterValue<size_t>("toydet:nchannels");

        // extract parameters useful for printing messages to screen
        m_print_freq    = app->GetParameterValue<size_t>("toydet:msg_print_freq");
        m_sub_socket    = app->GetParameterValue<std::string>("toydet:sub_socket");

        // Allocate the buffer. The buffer size must be determinable at startup time and
        // constant for the life of the program. If we are a consumer, the buffer should be large
        // enough to accept any message emitted from our producer. This object won't ever reallocate the buffer.
        // The factor five comes from the 4 ADC bytes per sample plus the 1 space delimiter or newline character
        // Each buffer contains 1024 samples (single readout window) from 80 channels, which is dubbed an 'event'
        m_buffer_capacity = sizeof(INDRAMessage) + 5 * (m_sample_count * m_channel_count);
        m_buffer = new char[m_buffer_capacity];

    }

    /// This constructor won't be used by JStreamingEventSource, but will instead be used when DASEventMessages are
    /// 'manually' created by DummyProducers and JEventProcessors
    explicit DASEventMessage(size_t payload_bytes) {
        m_buffer_capacity = sizeof(INDRAMessage) + payload_bytes;
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

    size_t get_event_number() const override { return as_indra_message()->record_counter; }
    size_t get_run_number() const override { return 1; }
    bool is_end_of_stream() const override { return as_indra_message()->flags == 1; }
    char *as_buffer() override { return m_buffer; }
    const char *as_buffer() const override { return m_buffer; }
    size_t get_buffer_capacity() const override { return m_buffer_capacity; }
    size_t get_buffer_size() const override { return sizeof(INDRAMessage) + as_indra_message()->payload_bytes; }

    ////////////////////////////////////////////////////////////////////////////////////////
    /// The following setters are NOT required by JStreamingEventSource, but useful for writing producers.
    /// It is always advisable to put the code for the setters close to the code for the getters.

    void set_end_of_stream() { as_indra_message()->flags = 1; }
    void set_event_number(size_t event_number) { as_indra_message()->record_counter = event_number; }
    static void set_run_number(size_t run_number) { ; }

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
    INDRAMessage *as_indra_message() { return reinterpret_cast<INDRAMessage *>(m_buffer); }

    /// Grants read-only access to any INDRAMessage members directly
    const INDRAMessage *as_indra_message() const { return reinterpret_cast<INDRAMessage *>(m_buffer); }

    /// Grants read-only access to the message payload as a byte array, which we need because INDRAMessage uses uint32_t instead
    void as_payload(const char **payload, size_t *payload_bytes) const {

        *payload = m_buffer + sizeof(INDRAMessage);
        *payload_bytes = as_indra_message()->payload_bytes;
    }

    /// Grants read/write access to the message payload as a byte array, which we need because INDRAMessage uses uint32_t instead
    void as_payload(char **payload, size_t *payload_bytes, size_t *payload_capacity) {

        *payload = m_buffer + sizeof(INDRAMessage);
        *payload_bytes = as_indra_message()->payload_bytes;
        *payload_capacity = m_buffer_capacity - sizeof(INDRAMessage);
    }

    /// Sets a payload size, measured in bytes
    void set_payload_size(uint32_t payload_bytes) {

        if (payload_bytes > m_buffer_capacity) {
            throw JException("set_payload_size: desired size exceeds buffer capacity!");
        }
        as_indra_message()->payload_bytes = payload_bytes;
    }

    /// Conveniently access message properties
    size_t get_sample_count() const { return m_sample_count; }
    size_t get_channel_count() const { return m_channel_count; }
    size_t get_message_print_freq() const { return m_print_freq; }
    std::string get_sub_socket() const { return m_sub_socket; }

private:

    size_t m_sample_count{};
    size_t m_channel_count{};
    char *m_buffer;
    size_t m_buffer_capacity{};
    size_t m_print_freq{};
    std::string m_sub_socket{};

};

/// Conveniently print a one-line summary of any DASEventMessage for debugging
inline std::ostream& operator<< (std::ostream& os, const DASEventMessage& message) {

    std::stringstream ss;
    const char* payload;
    size_t length;
    message.as_payload(&payload, &length);
    size_t eventNum  = message.get_event_number();
    size_t msgFreq   = message.get_message_print_freq();
    size_t buffSize  = message.get_buffer_size();
    string subSocket = message.get_sub_socket();
    if (eventNum % msgFreq == 0) {
        ss << "INDRA Message Recieved on socket " << subSocket
           << " -> Event " << eventNum
           << ", buffer size = " << buffSize
           << ", payload = ";
        for (int i = 0; i < 10 && i < (int) length; ++i) {
            ss << payload[i] << ", ";
        }
        ss << "...";
        os << ss.str() << std::endl;
    }
    return os;
}

#endif // _INDRAMessage_h_
