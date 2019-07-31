//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JMESSAGE_H
#define JANA2_JMESSAGE_H

#include <JANA/JObject.h>
#include <cstring>


/// JMessage is an abstract base class for a message type for streamed data,
/// usually corresponding to detector hits.
/// JMessages must be JObjects, so that they can be inserted into a JEvent aggregate.
/// They also must be able to provide a source_id and timestamp, which are needed
/// by JWindow in order to do event building, as described in JEventBuilder.
struct JMessage : public JObject {

    using DetectorId = uint64_t;  // TODO: Choose these types more carefully
    using Timestamp = uint64_t;

    virtual DetectorId get_source_id() = 0;
    virtual Timestamp get_timestamp() = 0;
    virtual bool is_end_of_stream() = 0;
    virtual size_t get_buffer_size() = 0;
    virtual size_t get_max_buffer_size() = 0;

    virtual void deserialize(char* buffer, size_t length) {
        memcpy((void*) this, (void*) buffer, length);
    };

    virtual void serialize(char** buffer, size_t* length) const {
        *length = sizeof(*this);
        *buffer = (char*) malloc(*length);
        memcpy((void*) buffer, (void*) this, *length);
    };
};


#endif //JANA2_JMESSAGE_H
