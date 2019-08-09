//
// Created by Nathan Brei on 2019-07-29.
//

#ifndef JANA2_JMESSAGE_H
#define JANA2_JMESSAGE_H

#include <JANA/JObject.h>


using DetectorId = uint64_t;
using Timestamp = uint64_t;

/// JMessage is an abstract base class for a message type for streamed data,
/// usually corresponding to detector hits.
/// JMessages must be JObjects, so that they can be inserted into a JEvent aggregate.
/// They also must be able to provide a source_id and timestamp, which are needed
/// by JWindow in order to do event building, as described in JEventBuilder.
struct JMessage : public JObject {

    virtual DetectorId get_source_id() const = 0;
    virtual Timestamp get_timestamp() const = 0;

    virtual char* as_buffer() = 0;
    virtual const char* as_buffer() const = 0;
    virtual size_t get_buffer_size() const = 0;
    virtual size_t get_data_size() const = 0;
    virtual bool is_end_of_stream() const = 0;
};


#endif //JANA2_JMESSAGE_H
